/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/kad_impl.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/crypto/sha/sha256.hpp>
//#include <libp2p/protocol/kademlia/query.hpp>


OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol::kademlia, Kad::Error, e) {
  using E = libp2p::protocol::kademlia::Kad::Error;
  switch (e) {
    case E::SUCCESS:
      return "success";
    case E::NO_PEERS:
      return "no peers found";
    case E::MESSAGE_PARSE_ERROR:
      return "message deserialize error";
    case E::MESSAGE_SERIALIZE_ERROR:
      return "message serialize error";
    default:
      break;
  }
  return "unknown error";
}

namespace {

  using libp2p::common::Hash256;
  using libp2p::crypto::sha256;
  using libp2p::peer::PeerId;
  using libp2p::protocol::kademlia::NodeId;

  NodeId hash(const PeerId &pid) {
    return NodeId(sha256(pid.toVector()));
  }

}  // namespace

namespace libp2p::protocol::kademlia {
/*
  void KadImpl::findPeer(peer::PeerId id, PeerRouting::FindPeerResultFunc f) {
    auto local = findLocal(id);
    auto connectedness = network_.getConnectionManager().connectedness(local);
    using E = network::ConnectionManager::Connectedness;
    switch (connectedness) {
      case E::CAN_CONNECT:
      case E::CONNECTED:
        // we know addresses of that peer, return local view
        return f(local);
      default:
        // we don't know addresses of that peer. continue.
        break;
    }

    auto peers = table_->getNearestPeers(hash(id), config_.ALPHA);
    if (peers.empty()) {
      // can not find nearest peers...
      return f(Error::NO_PEERS);
    }

    for (peer::PeerId &p : peers) {
      if (p == id) {
        // found target peer in list of closest peers
        return f(findLocal(id));
      }
    }

    // create query
    Query query{
        id.toVector(),  /// find this peer
        [this, id](const Key &key,
                   std::function<void(outcome::result<QueryResult>)> handle) {
          mrw_->findPeerSingle(
              key,  /// ask this peer (serialized NodeId)
              id,   /// where peer 'id' is
              [id, handle{std::move(handle)}](outcome::result<PeerInfoVec> r) {
                if (!r) {
                  return handle(r.error());
                }

                // see if we got the peer here
                auto &&list = r.value();
                for (const auto &p : list) {
                  if (p.id == id) {
                    // yey!
                    QueryResult q;
                    q.peer = p;
                    q.success = true;
                    return handle(std::move(q));
                  }
                }

                QueryResult q;
                q.closerPeers = list;
                return handle(std::move(q));
              });
        }};

    return runner_->run(std::move(query), std::move(peers),
                        std::bind(f, std::placeholders::_1));
  }
*/
  peer::PeerInfo KadImpl::findLocal(const peer::PeerId &id) {
    auto r = repo_->getAddressRepository().getAddresses(id);
    if (r) {
      return {id, r.value()};
    }

    return {id, {}};
  }

  peer::Protocol KadImpl::getProtocolId() const {
    return peer::Protocol(protocolId);
  }

  void KadImpl::handle(StreamResult rstream) {
    if (!rstream) {
      log_->info("incoming connection failed due to '{}'",
                 rstream.error().message());
      return;
    }

    auto session = server_session_create_fn_();
    auto session_id = ++sessions_counter_;
    session->start(
      rstream.value(),
      [this, session_id](outcome::result<Message> result) {
        onMessage(session_id, std::move(result));
      }
    );

    sessions_[session_id] = { rstream.value(), std::move(session) };
  }

  void KadImpl::onMessage(uint64_t from, outcome::result<Message> msg) {
    bool close = false;
    if (!msg) {
      close = true;
      log_->warn("{}", msg.error().message()); //TODO
    } else {
      if (msg.value().type == Message::kFindNode) {
        if (msg.value().key.size() != 32) { //????
          close = true;
        } else {
          replyToFindNode(from, std::move(msg.value()));
        }

      } else {
        close = true;
        log_->warn("msg type {} handler NYI", msg.value().type); //TODO
      }
    }

    if (close) {
      closeSession(from);
    }
  }

  void KadImpl::replyToFindNode(uint64_t to, Message&& msg) {
    auto it = sessions_.find(to);
    if (it == sessions_.end()) {
      log_->warn("cannot find session id={}", to);
      return;
    }
    Session& session = it->second;

    NodeId id(msg.key.data());
    auto peerIds = table_->getNearestPeers(id, config_.ALPHA*2);
    msg.closer_peers = Message::Peers();
    for (const auto& id : peerIds) {
      msg.closer_peers.value().push_back({ repo_->getPeerInfo(id), network::ConnectionManager::Connectedness::CAN_CONNECT } );
    }

    session.session->reply(msg);
  }

  void KadImpl::closeSession(uint64_t id) {
    auto it = sessions_.find(id);
    if (it == sessions_.end()) {
      log_->warn("cannot find session id={}", id);
      return;
    }
    it->second.stream->close([](outcome::result<void>){}); //???
    it->second.session->stop();
    sessions_.erase(it);
  }

  KadImpl::KadImpl(network::Network &network,
                   std::shared_ptr<peer::PeerRepository> repo,
                   std::shared_ptr<RoutingTable> table,
                   //std::shared_ptr<MessageReadWriter> mrw,
                   //std::shared_ptr<QueryRunner> runner,
                   KademliaConfig config,
                   KadServerSessionCreate server_session_create)
      : network_(network),
        repo_(std::move(repo)),
        table_(std::move(table)),
        //mrw_(std::move(mrw)),
        //runner_(std::move(runner)),
        config_(config) {
    BOOST_ASSERT(repo_ != nullptr);
    BOOST_ASSERT(table_ != nullptr);
  //  BOOST_ASSERT(mrw_ != nullptr);
  //  BOOST_ASSERT(runner_ != nullptr);

    if (!server_session_create) {
      server_session_create_fn_ = std::move(server_session_create);
    } else {
      server_session_create_fn_ = &KadServerSessionImpl::create;
    }
  }
}  // namespace libp2p::protocol::kademlia
