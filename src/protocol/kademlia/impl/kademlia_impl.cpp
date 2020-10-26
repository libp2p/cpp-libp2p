/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/types.hpp>
#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/peer/address_repository.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/peer_finder.hpp>
#include <libp2p/protocol/kademlia/message.hpp>
#include <unordered_set>

#include "libp2p/protocol/kademlia/impl/kademlia_impl.hpp"

namespace libp2p::protocol::kademlia {

  KademliaImpl::KademliaImpl(
      const Config &config, std::shared_ptr<Host> host,
      std::shared_ptr<ValueStore> storage, std::shared_ptr<RoutingTable> table,
      std::shared_ptr<MessageObserver> message_observer,
      std::shared_ptr<Scheduler> scheduler,
      std::shared_ptr<crypto::random::RandomGenerator> random_generator)
      : config_(config),
        host_(std::move(host)),
        storage_(std::move(storage)),
        table_(std::move(table)),
        message_observer_(std::move(message_observer)),
        scheduler_(std::move(scheduler)),
        random_generator_(std::move(random_generator)),
        protocol_(config_.protocolId),
        self_id_(host_->getId()),
        log_("kad", "Kademlia", this) {}

  void KademliaImpl::start() {
    BOOST_ASSERT(not started_);
    if (started_) {
      return;
    }

    // subscribe to new connection
    new_channel_subscription_ =
        host_->getBus()
            .getChannel<network::event::OnNewConnectionChannel>()
            .subscribe([this](
                           // NOLINTNEXTLINE
                           std::weak_ptr<connection::CapableConnection> conn) {
              if (auto c = conn.lock()) {
                // adding outbound connections only
                if (c->isInitiator()) {
                  log_.debug("new outbound connection");
                  auto remote_peer_res = c->remotePeer();
                  if (!remote_peer_res) {
                    return;
                  }
                  auto remote_peer_addr_res = c->remoteMultiaddr();
                  if (!remote_peer_addr_res) {
                    return;
                  }
                  addPeer(
                      peer::PeerInfo{std::move(remote_peer_res.value()),
                                     {std::move(remote_peer_addr_res.value())}},
                      false);
                }
              }
            });

    // start random walking
    if (config_.randomWalk.enabled) {
      randomWalk();
    }
  }

  void KademliaImpl::addPeer(const PeerInfo &peer_info, bool permanent) {
    auto res =
        host_->getPeerRepository().getAddressRepository().upsertAddresses(
            peer_info.id,
            gsl::span(peer_info.addresses.data(), peer_info.addresses.size()),
            permanent ? peer::ttl::kPermanent : peer::ttl::kDay);
    if (res) {
      res = table_->update(peer_info.id);
    }
    std::string id_str = peer_info.id.toBase58();
    if (res) {
      log_.debug("successfully added peer to table: {}", id_str);
    } else {
      log_.debug("failed to add peer to table: {} : {}", id_str,
                 res.error().message());
    }
  }

  std::vector<PeerId> KademliaImpl::getNearestPeers(const NodeId &id) {
    return table_->getNearestPeers(id, config_.closerPeerCount * 2);
  }

  outcome::result<void> KademliaImpl::findPeer(const peer::PeerId &peer_id,
                                               FoundPeerInfoHandler handler) {
    log_.debug("new findPeer request, peer_id = {}", peer_id.toBase58());

    // Try to find locally
    auto peer_info = host_->getPeerRepository().getPeerInfo(peer_id);
    if (!peer_info.addresses.empty()) {
      scheduler_
          ->schedule([handler = std::move(handler),
                      peer_info = std::move(peer_info)] { handler(peer_info); })
          .detach();

      log_.info("{} found locally from host!", peer_id.toBase58());
      return outcome::success();
    }

    // Get nearest peer
    auto nearest_peer_ids = table_->getNearestPeers(NodeId(peer_id), 20);
    if (nearest_peer_ids.empty()) {
      log_.info("{} : no peers", peer_id.toBase58());
      return Error::NO_PEERS;
    }

    std::unordered_set<PeerInfo> nearest_peer_infos;

    // Try to find over nearest peers
    for (const auto &nearest_peer_id : nearest_peer_ids) {
      // Exclude yoursef
      if (nearest_peer_id == self_id_) {
        continue;
      }
      // Get peer info
      auto info = host_->getPeerRepository().getPeerInfo(nearest_peer_id);
      if (info.addresses.empty()) {
        continue;
      }

      // Check if connectable
      auto connectedness =
          host_->getNetwork().getConnectionManager().connectedness(info);
      if (connectedness == Message::Connectedness::CONNECTED
          || connectedness == Message::Connectedness::CAN_CONNECT) {
        nearest_peer_infos.insert(std::move(info));
      }
    }

    if (nearest_peer_infos.empty()) {
      log_.info("{} : no peers to connect to", peer_id.toBase58());
      return Error::NO_PEERS;
    }

    boost::optional<peer::PeerInfo> self_announce;
    peer::PeerInfo self = host_->getPeerInfo();
    if (started_) {
      self_announce = self;
    }

    auto peer_finder = std::make_shared<PeerFinder>(
        host_, self_announce, peer_id, nearest_peer_infos, handler);

    return peer_finder->start();

    //  Message request = createFindNodeRequest(peer_id,
    //  std::move(self_announce));
    //
    //  auto buffer = std::make_shared<std::vector<uint8_t>>();
    //  if (!request.serialize(*buffer)) {
    //    log_.error("serialize error");
    //    return Error::MESSAGE_SERIALIZE_ERROR;
    //  }
    //
    //  FindPeerQueryResultFunc response_handler = [handler =
    //  std::move(handler)](
    //                                                 const PeerId &peer,
    //                                                 FindPeerQueryResult
    //                                                 result) {
    //    // TODO(xDimon): Need to check if works in acconding to spec
    //    if (result.success) {
    //      handler(std::move(result.peer.value()));
    //    } else {
    //      handler(Error::VALUE_NOT_FOUND);
    //    }
    //  };
    //
    //  OUTCOME_TRY(
    //      findPeer(peer_id, nearest_peer_infos, std::move(response_handler)));
    //
    //  return Error::IN_PROGRESS;
  }

  void KademliaImpl::findRandomPeer() {
    BOOST_ASSERT(config_.randomWalk.enabled);

    common::Hash256 hash;
    random_generator_->fillRandomly(hash);

    auto peer_id =
        peer::PeerId::fromHash(
            multi::Multihash::create(multi::HashType::sha256, hash).value())
            .value();

    //    KademliaImpl::FindPeerQueryResultFunc handler =
    //        [this](const peer::PeerId &peer, FindPeerQueryResult result) {
    //          if (result.success) {
    //            addPeer(result.peer.get(), false);
    //          } else {
    //            for (auto &peer_info : result.closer_peers) {
    //              addPeer(peer_info, false);
    //            }
    //          }
    //        };

    FoundPeerInfoHandler handler =
        [wp = weak_from_this()](outcome::result<PeerInfo> res) {
          if (auto self = wp.lock()) {
            if (res.has_value()) {
              self->addPeer(res.value(), false);
            }
          }
        };

    [[maybe_unused]] auto res = findPeer(peer_id, handler);
  }

  void KademliaImpl::randomWalk() {
    BOOST_ASSERT(config_.randomWalk.enabled);

    // Doing walk
    findRandomPeer();

    auto iteration = random_walking_.iteration++;

    // if period end
    scheduler::Ticks delay =
        ((iteration % config_.randomWalk.queries_per_period) != 0)
        ? scheduler::toTicks(config_.randomWalk.delay)
        : scheduler::toTicks(config_.randomWalk.interval
                             - config_.randomWalk.delay
                                 * config_.randomWalk.queries_per_period);

    // Schedule next walking
    random_walking_.handle =
        scheduler_->schedule(delay, [this] { randomWalk(); });
  }
}  // namespace libp2p::protocol::kademlia
