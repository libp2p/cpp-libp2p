/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/kademlia/impl/peer_finder.hpp"
#include <libp2p/protocol/kademlia/error.hpp>
#include <libp2p/protocol/kademlia/impl/session.hpp>
#include <libp2p/protocol/kademlia/message.hpp>
#include <libp2p/protocol/kademlia/peer_routing.hpp>

namespace libp2p::protocol::kademlia {

  PeerFinder::PeerFinder(std::shared_ptr<Host> host,
                         boost::optional<peer::PeerInfo> self_peer_info,
                         PeerId sought_peer_id,
                         std::unordered_set<PeerInfo> nearest_peer_infos,
                         FoundPeerInfoHandler handler)
      : host_(std::move(host)),
        self_peer_info_(std::move(self_peer_info)),
        sought_peer_id_(std::move(sought_peer_id)),
        nearest_peer_infos_(std::move(nearest_peer_infos)),
        handler_(std::move(handler)) {
    std::for_each(nearest_peer_infos.begin(), nearest_peer_infos.end(),
                  [this](auto &peer_info) {
                    queue_.emplace(peer_info, sought_peer_id_);
                  });
  }

  outcome::result<void> PeerFinder::start() {
    if (started_) {
      return Error::IN_PROGRESS;
    }
    if (done_) {
      return Error::FULFILLED;
    }
    started_ = true;

    Message request = createFindNodeRequest(sought_peer_id_, self_peer_info_);
    if (!request.serialize(serialized_request_)) {
      return Error::MESSAGE_SERIALIZE_ERROR;
    }

    spawn();

    return outcome::success();
  };

  void PeerFinder::spawn() {
    while (not queue_.empty()
           and requests_in_progress_ < max_concurent_requests_) {
      auto &peer_info = *queue_.top();
      queue_.pop();

      auto session_id = ++connecting_sessions_counter_;

      //    log_.debug("connecting to {}, {}", closest_peer_info.id.toBase58(),
      //    handler.use_count());

      host_->newStream(peer_info, "protocol_",
                       [wp = weak_from_this(), session_id,
                        peer_id = peer_info.id](auto &&stream_res) {
                         auto self = wp.lock();
                         if (self) {
                           self->onConnected(
                               session_id, peer_id,
                               std::forward<decltype(stream_res)>(stream_res));
                         }
                       });

      //    connecting_sessions_[id] = handler;

      ++requests_in_progress_;
    }

    if (requests_in_progress_ == 0) {
      handler_(Error::VALUE_NOT_FOUND);
    }
  }

  void PeerFinder::onConnected(
      uint64_t session_id, const peer::PeerId &peerId,
      outcome::result<std::shared_ptr<connection::Stream>> stream_res) {
    //	auto it = connecting_sessions_.find(session_id);
    //	if (it == connecting_sessions_.end()) {
    //		log_.warn("cannot find connecting session {}", session_id);
    //		return;
    //	}
    //	auto handler = std::move(it->second);
    //	connecting_sessions_.erase(it);

    if (!stream_res) {
      // TODO(xDimon):
      //  log_.warn("cannot connect to server: {}",
      //  stream_res.error().message());
      --requests_in_progress_;
      spawn();
      return;
    }

    auto stream = stream_res.value().get();
    assert(stream->remoteMultiaddr());
    //	assert(sessions_.find(stream) == sessions_.end());

    //	std::string addr(stream->remoteMultiaddr().value().getStringAddress());
    //	log_.debug("connected to {}, ({} - {})", addr,
    //	           stream_res.value().use_count(),
    //	           connecting_sessions_.size());

    auto session = std::make_shared<Session>(weak_from_this(),
                                             std::move(stream_res.value()));

    if (!session->write(serialized_request_)) {
      //		log_.warn("write to {} failed", addr);
      assert(stream->remotePeerId());

      --requests_in_progress_;
      spawn();
      return;
    }

    session->state(Session::State::writing_to_peer);

    sessions_[stream] = std::move(session);
    //    log_.debug("total sessions: {}", sessions_.size());
  }

  void PeerFinder::onCompleted(connection::Stream *from,
                            outcome::result<void> res) {
	  auto it = sessions_.find(from);
	  if (it == sessions_.end()) {
//		  log_.warn("cannot find session by stream");
		  return;
	  }
		auto session = it->second;

//    if (session->response_handler->needResponse()) {
//      if (res.error() == Error::SUCCESS
//          && session->protocol_handler->state() == writing_to_peer) {
//        // request has been written, wait for response
//        if (session->protocol_handler->read()) {
//          session->protocol_handler->state(reading_from_peer);
//          return;
//        }
//        res = outcome::failure(Error::STREAM_RESET);
//      }
//
//      auto peer_res = from->remotePeerId();
//      assert(peer_res);
//
//      session->response_handler->onResult(peer_res.value(),
//                                          outcome::failure(res.error()));
//    }

//    log_.debug("client session completed, total sessions: {}",
//               sessions_.size() - 1);

	  session->close();
	  sessions_.erase(it);
  }

}  // namespace libp2p::protocol::kademlia
