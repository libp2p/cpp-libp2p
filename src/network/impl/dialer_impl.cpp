/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <functional>
#include <iostream>

#include <libp2p/connection/stream.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/network/impl/dialer_impl.hpp>

namespace libp2p::network {

  void DialerImpl::dial(const peer::PeerInfo &p, DialResultFunc cb,
                        std::chrono::milliseconds timeout) {
    SL_TRACE(log_, "Dialing to {}", p.id.toBase58().substr(46));
    if (auto c = cmgr_->getBestConnectionForPeer(p.id); c != nullptr) {
      // we have connection to this peer

      SL_TRACE(log_, "Reusing connection to peer {}",
               p.id.toBase58().substr(46));
      scheduler_->schedule(
          [cb{std::move(cb)}, c{std::move(c)}]() mutable { cb(std::move(c)); });
      return;
    }

    if (auto ctx = dialing_peers_.find(p.id); dialing_peers_.end() != ctx) {
      SL_TRACE(log_, "Dialing to {} is already in progress",
               p.id.toBase58().substr(46));
      // populate known addresses for in-progress dial if any new appear
      for (const auto &addr : p.addresses) {
        if (0 == ctx->second.tried_addresses.count(addr)) {
          ctx->second.addresses.insert(addr);
        }
      }
      ctx->second.callbacks.emplace_back(std::move(cb));
      return;
    }

    // we don't have a connection to this peer.
    // did user supply its addresses in {@param p}?
    if (p.addresses.empty()) {
      // we don't have addresses of peer p
      scheduler_->schedule(
          [cb{std::move(cb)}] { cb(std::errc::destination_address_required); });
      return;
    }

    DialCtx new_ctx{.addresses = {p.addresses.begin(), p.addresses.end()},
                    .timeout = timeout};
    new_ctx.callbacks.emplace_back(std::move(cb));
    bool scheduled = dialing_peers_.emplace(p.id, std::move(new_ctx)).second;
    BOOST_ASSERT(scheduled);
    rotate(p.id);
  }

  void DialerImpl::rotate(const peer::PeerId &peer_id) {
    auto ctx_found = dialing_peers_.find(peer_id);
    if (dialing_peers_.end() == ctx_found) {
      SL_ERROR(log_, "State inconsistency - cannot dial {}",
               peer_id.toBase58());
      return;
    }
    auto &&ctx = ctx_found->second;

    if (ctx.addresses.empty() and not ctx.dialled) {
      completeDial(peer_id, std::errc::address_family_not_supported);
      return;
    }
    if (ctx.addresses.empty() and ctx.result.has_value()) {
      completeDial(peer_id, ctx.result.value());
      return;
    }
    if (ctx.addresses.empty()) {
      // this would never happen. Previous if-statement should work instead'
      completeDial(peer_id, std::errc::host_unreachable);
      return;
    }

    auto dial_handler =
        [wp{weak_from_this()}, peer_id](
            outcome::result<std::shared_ptr<connection::CapableConnection>>
                result) {
          if (auto self = wp.lock()) {
            auto ctx_found = self->dialing_peers_.find(peer_id);
            if (self->dialing_peers_.end() == ctx_found) {
              SL_ERROR(
                  self->log_,
                  "State inconsistency - uninteresting dial result for peer {}",
                  peer_id.toBase58());
              if (result.has_value() and not result.value()->isClosed()) {
                auto close_res = result.value()->close();
                BOOST_ASSERT(close_res);
              }
              return;
            }

            if (result.has_value()) {
              self->listener_->onConnection(result);
              self->completeDial(peer_id, result);
              return;
            }

            // store an error otherwise and reschedule one more rotate
            ctx_found->second.result = std::move(result);
            self->scheduler_->schedule([wp, peer_id] {
              if (auto self = wp.lock()) {
                self->rotate(peer_id);
              }
            });
            return;
          }
          // closing the connection when dialer and connection requester
          // callback no more exist
          if (result.has_value() and not result.value()->isClosed()) {
            auto close_res = result.value()->close();
            BOOST_ASSERT(close_res);
          }
        };

    auto first_addr = ctx.addresses.begin();
    const auto addr = *first_addr;
    ctx.tried_addresses.insert(addr);
    ctx.addresses.erase(first_addr);
    if (auto tr = tmgr_->findBest(addr); nullptr != tr) {
      ctx.dialled = true;
      SL_TRACE(log_, "Dial to {} via {}", peer_id.toBase58().substr(46),
               addr.getStringAddress());
      tr->dial(peer_id, addr, dial_handler, ctx.timeout);
    } else {
      scheduler_->schedule([wp{weak_from_this()}, peer_id] {
        if (auto self = wp.lock()) {
          self->rotate(peer_id);
        }
      });
    }
  }

  void DialerImpl::completeDial(const peer::PeerId &peer_id,
                                const DialResult &result) {
    if (auto ctx_found = dialing_peers_.find(peer_id);
        dialing_peers_.end() != ctx_found) {
      auto &&ctx = ctx_found->second;
      for (auto i = 0u; i < ctx.callbacks.size(); ++i) {
        scheduler_->schedule(
            [result, cb{std::move(ctx.callbacks[i])}] { cb(result); });
      }
      dialing_peers_.erase(ctx_found);
    }
  }

  void DialerImpl::newStream(const peer::PeerInfo &p, StreamProtocols protocols,
                             StreamAndProtocolOrErrorCb cb,
                             std::chrono::milliseconds timeout) {
    SL_TRACE(log_, "New stream to {} for {} (peer info)",
             p.id.toBase58().substr(46), fmt::join(protocols, " "));
    dial(
        p,
        [self{shared_from_this()}, protocols{std::move(protocols)},
         cb{std::move(cb)}](
            outcome::result<std::shared_ptr<connection::CapableConnection>>
                rconn) mutable {
          if (!rconn) {
            return cb(rconn.error());
          }
          auto &&conn = rconn.value();
          self->newStream(std::move(conn), std::move(protocols), std::move(cb));
        },
        timeout);
  }

  void DialerImpl::newStream(const peer::PeerId &peer_id,
                             StreamProtocols protocols,
                             StreamAndProtocolOrErrorCb cb) {
    SL_TRACE(log_, "New stream to {} for {} (peer id)",
             peer_id.toBase58().substr(46), fmt::join(protocols, " "));
    auto conn = cmgr_->getBestConnectionForPeer(peer_id);
    if (!conn) {
      scheduler_->schedule(
          [cb{std::move(cb)}] { cb(std::errc::not_connected); });
      return;
    }
    newStream(std::move(conn), std::move(protocols), std::move(cb));
  }

  void DialerImpl::newStream(
      std::shared_ptr<connection::CapableConnection> conn,
      StreamProtocols protocols, StreamAndProtocolOrErrorCb cb) {
    auto stream_res = conn->newStream();
    if (stream_res.has_error()) {
      scheduler_->schedule(
          [cb{std::move(cb)}, error{stream_res.error()}] { cb(error); });
      return;
    }
    auto &&stream = stream_res.value();
    auto stream_copy = stream;
    multiselect_->selectOneOf(
        protocols, std::move(stream_copy), true, true,
        [stream{std::move(stream)}, cb{std::move(cb)}](
            outcome::result<peer::ProtocolName> protocol_res) mutable {
          if (protocol_res.has_error()) {
            return cb(protocol_res.error());
          }
          auto &&protocol = protocol_res.value();
          cb(StreamAndProtocol{std::move(stream), std::move(protocol)});
        });
  }

  DialerImpl::DialerImpl(
      std::shared_ptr<protocol_muxer::ProtocolMuxer> multiselect,
      std::shared_ptr<TransportManager> tmgr,
      std::shared_ptr<ConnectionManager> cmgr,
      std::shared_ptr<ListenerManager> listener,
      std::shared_ptr<basic::Scheduler> scheduler)
      : multiselect_(std::move(multiselect)),
        tmgr_(std::move(tmgr)),
        cmgr_(std::move(cmgr)),
        listener_(std::move(listener)),
        scheduler_(std::move(scheduler)),
        log_(log::createLogger("DialerImpl")) {
    BOOST_ASSERT(multiselect_ != nullptr);
    BOOST_ASSERT(tmgr_ != nullptr);
    BOOST_ASSERT(cmgr_ != nullptr);
    BOOST_ASSERT(listener_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(log_ != nullptr);
  }

}  // namespace libp2p::network
