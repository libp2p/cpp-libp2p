/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_PEERFINDER
#define LIBP2P_PROTOCOL_KADEMLIA_PEERFINDER

#include <memory>
#include <queue>
#include <unordered_set>

#include <libp2p/common/types.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>

namespace libp2p::protocol::kademlia {

  namespace {

    struct PeerInfoWithDistance {
      PeerInfoWithDistance(const PeerInfo &peer_info, PeerId target_peer_id)
          : peer_info_(peer_info) {
        auto &p1 = peer_info.id.toVector();
        auto &p2 = target_peer_id.toVector();
        for (size_t i = 0u; i < distance_.size(); ++i) {
          distance_[i] = p1[i] ^ p2[i];
        }
      }

      bool operator<(const PeerInfoWithDistance &other) const noexcept {
        return std::memcmp(distance_.data(), other.distance_.data(),
                           distance_.size())
            < 0;
      }

      const PeerInfo &operator*() const {
        return peer_info_.get();
      }

      std::reference_wrapper<const PeerInfo> peer_info_;
      common::Hash256 distance_;
    };

  }  // namespace

  class PeerFinder : public std::enable_shared_from_this<PeerFinder> {
   public:
    PeerFinder(std::shared_ptr<Host> host,
               boost::optional<peer::PeerInfo> self_peer_info,
               PeerId sought_peer_id,
               std::unordered_set<PeerInfo> nearest_peer_infos,
               FoundPeerInfoHandler handler);

    outcome::result<void> start();

//   private:
    void spawn();
    void onConnected(
        uint64_t session_id, const peer::PeerId &peerId,
        outcome::result<std::shared_ptr<connection::Stream>> stream_res);
    void onResponse();

		void onCompleted(connection::Stream *from,
		                               outcome::result<void> res);


			  std::shared_ptr<Host> host_;
    boost::optional<peer::PeerInfo> self_peer_info_;
    PeerId sought_peer_id_;
    std::unordered_set<PeerInfo> nearest_peer_infos_;
    FoundPeerInfoHandler handler_;

    std::vector<uint8_t> serialized_request_;
    std::priority_queue<PeerInfoWithDistance> queue_;
    size_t connecting_sessions_counter_ = 0;
    size_t requests_in_progress_ = 0;
    size_t max_concurent_requests_ = 0;
    bool started_ = false;
    bool done_ = false;

    std::map<connection::Stream *, std::shared_ptr<Session>> sessions_;
  };

//  class FindPeerBatchHandler : public KadResponseHandler {
//   public:
//    FindPeerBatchHandler(peer::PeerId self, peer::PeerId key,
//                         Kad::FindPeerQueryResultFunc f, Kad &kad)
//        : self_(std::move(self)),
//          key_(std::move(key)),
//          callback_(std::move(f)),
//          kad_(kad),
//          log_("kad", "FindPeerBatchHandler", &kad) {}
//
//    void wait_for(const peer::PeerId &id) {
//      waiting_for_.insert(id);
//    }
//
//   private:
//    Message::Type expectedResponseType() override {
//      return Message::kFindNode;
//    }
//
//    bool needResponse() override {
//      return true;
//    }
//
//    void onResult(const peer::PeerId &from,
//                  outcome::result<Message> result) override {
//      log_.debug("{} waiting for {} responses", from.toBase58(),
//                 waiting_for_.size());
//      waiting_for_.erase(from);
//      if (!result) {
//        log_.warn("request to {} failed: {}", from.toBase58(),
//                  result.error().message());
//      } else {
//        Message &msg = result.value();
//        size_t n = 0;
//        if (msg.closer_peers) {
//          n = msg.closer_peers.value().size();
//          for (auto &p : msg.closer_peers.value()) {
//            if (p.info.id == self_) {
//              --n;
//              continue;
//            }
//            if (p.conn_status != Message::Connectedness::CAN_NOT_CONNECT
//                && p.conn_status != Message::Connectedness::NOT_CONNECTED) {
//              if (callback_) {
//                if (p.info.id == key_) {
//                  result_.success = true;
//                  result_.peer = p.info;
//                }
//                result_.closer_peers.insert(p.info);
//              }
//              kad_.addPeer(std::move(p.info), false);
//            }
//          }
//        }
//        log_.debug("{} returned {} records, waiting for {} responses",
//                   from.toBase58(), n, waiting_for_.size());
//      }
//      if (result_.success || waiting_for_.empty()) {
//        if (callback_) {
//          callback_(key_, std::move(result_));
//          callback_ = nullptr;
//          if (waiting_for_.empty()) {
//            log_.debug("done");
//          }
//        } else {
//          log_.debug("skipped");
//        }
//      } else {
//        log_.debug("waiting...");
//      }
//    }
//
//    peer::PeerId self_;
//    peer::PeerId key_;
//    Kad::FindPeerQueryResultFunc callback_;
//    Kad &kad_;
//    Kad::FindPeerQueryResult result_;
//    std::unordered_set<peer::PeerId> waiting_for_;
//    SubLogger log_;
//  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_PEERFINDER
