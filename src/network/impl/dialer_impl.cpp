/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/logger.hpp>
#include <libp2p/connection/stream.hpp>
#include <libp2p/network/impl/dialer_impl.hpp>

#define TRACE_ENABLED 0
#include <libp2p/common/trace.hpp>

namespace libp2p::network {

  void DialerImpl::dial(const peer::PeerInfo &p, DialResultFunc cb,
                        std::chrono::milliseconds timeout) {
    if (auto c = cmgr_->getBestConnectionForPeer(p.id); c != nullptr) {
      // we have connection to this peer

      TRACE("reusing connection to peer {}", p.id.toBase58().substr(46));
      cb(std::move(c));
      return;
    }

    // we don't have a connection to this peer.
    // did user supply its addresses in {@param p}?
    if (p.addresses.empty()) {
      // we don't have addresses of peer p
      cb(std::errc::destination_address_required);
      return;
    }

    struct DialHandlerCtx {
      bool connected;
      size_t calls_remain;
      DialResultFunc cb;

      DialHandlerCtx(size_t addresses, DialResultFunc cb)
          : connected(false), calls_remain(addresses), cb{std::move(cb)} {}
    };
    auto handler_ctx = std::make_shared<DialHandlerCtx>(p.addresses.size(), cb);
    auto dial_handler =
        [listener{listener_}, ctx{std::move(handler_ctx)}](
            outcome::result<std::shared_ptr<connection::CapableConnection>>
                connection_result) {
          --ctx->calls_remain;
          if (ctx->connected) {
            // we already got connected to the peer via some other address
            if (connection_result) {
              // lets close the redundant connection if so
              auto &&conn = connection_result.value();
              if (not conn->isClosed()) {
                auto close_res = conn->close();
                BOOST_ASSERT(close_res);
              }
            }  // otherwise we don't care about any failure since that was going
               // to be a redundant connection to the moment
            return;
          }

          if (connection_result) {
            // we've got the first successful connection to the peer, hooray!
            ctx->connected = true;
            // allow the connection accept inbound streams
            listener->onConnection(connection_result);
            // return connection to the user
            ctx->cb(connection_result.value());
            return;
          }

          // here we handle failed attempt to connect
          if (0 == ctx->calls_remain) {
            // that was the last attempt to connect and we are still not
            // connected so lets report an error to the user
            ctx->cb(connection_result.error());
            return;
          }  // otherwise we don't care about this particular failure because at
             // the end we either would get connected or will do the same -
             // report the error to the user inside the callback of the last
             // dial attempt
        };

    bool dialled{false};
    // for all multiaddresses supplied in peerinfo
    for (auto &&ma : p.addresses) {
      // try to find best possible transport
      if (auto tr = this->tmgr_->findBest(ma); tr != nullptr) {
        // we can dial to this peer!
        dialled = true;
        // dial using best transport
        tr->dial(p.id, ma, dial_handler, timeout);
        // All the dials are still to be executed sequentially within the single
        // boost::asio::io_context. Immediate spawn of all of them allows us to
        // have the closest timeout to the specified instead of
        // NumberOfMultiaddresses multiplied by a single timeout value.
      }
    }

    if (not dialled) {
      // we did not find supported transport
      cb(std::errc::address_family_not_supported);
    }
  }

  void DialerImpl::newStream(const peer::PeerInfo &p,
                             const peer::Protocol &protocol,
                             StreamResultFunc cb,
                             std::chrono::milliseconds timeout) {
    // 1. make new connection or reuse existing
    this->dial(
        p,
        [this, cb{std::move(cb)}, protocol](
            outcome::result<std::shared_ptr<connection::CapableConnection>>
                rconn) mutable {
          if (!rconn) {
            return cb(rconn.error());
          }
          auto &&conn = rconn.value();

          if (!conn->isInitiator()) {
            TRACE("dialer: opening outbound stream inside inbound connection");
          }

          // 2. open new stream on that connection
          conn->newStream(
              [this, cb{std::move(cb)},
               protocol](outcome::result<std::shared_ptr<connection::Stream>>
                             rstream) mutable {
                if (!rstream) {
                  return cb(rstream.error());
                }
                auto &&stream = rstream.value();

                TRACE("dialer: before multiselect");

                // 3. negotiate a protocol over that stream
                std::vector<peer::Protocol> protocols{protocol};
                this->multiselect_->selectOneOf(
                    protocols, stream, true /* initiator */,
                    [cb{std::move(cb)},
                     stream](outcome::result<peer::Protocol> rproto) mutable {
                      if (!rproto) {
                        return cb(rproto.error());
                      }

                      TRACE("dialer: inside multiselect callback");

                      // 4. return stream back to the user
                      cb(std::move(stream));
                    });
              });
        },
        timeout);
  }

  DialerImpl::DialerImpl(
      std::shared_ptr<protocol_muxer::ProtocolMuxer> multiselect,
      std::shared_ptr<TransportManager> tmgr,
      std::shared_ptr<ConnectionManager> cmgr,
      std::shared_ptr<ListenerManager> listener)
      : multiselect_(std::move(multiselect)),
        tmgr_(std::move(tmgr)),
        cmgr_(std::move(cmgr)),
        listener_(std::move(listener)) {
    BOOST_ASSERT(multiselect_ != nullptr);
    BOOST_ASSERT(tmgr_ != nullptr);
    BOOST_ASSERT(cmgr_ != nullptr);
    BOOST_ASSERT(listener_ != nullptr);
  }

}  // namespace libp2p::network
