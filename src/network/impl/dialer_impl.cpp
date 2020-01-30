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

  void DialerImpl::dial(const peer::PeerInfo &p, DialResultFunc cb) {
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

    // for all multiaddresses supplied in peerinfo
    for (auto &&ma : p.addresses) {
      // try to find best possible transport
      if (auto tr = this->tmgr_->findBest(ma); tr != nullptr) {
        // we can dial to this peer!
        // dial using best transport
        tr->dial(
            p.id, ma,
            [this, cb{std::move(cb)}, pid{p.id}](
                outcome::result<std::shared_ptr<connection::CapableConnection>>
                    rconn) {
              if (!rconn) {
                cb(rconn.error());
                return;
              }

              // allow the connection to accept inbound streams
              this->listener_->onConnection(rconn);

              // return connection to the user
              cb(rconn.value());
            });
        return;
      }
    }

    // we did not find supported transport
    cb(std::errc::address_family_not_supported);
  }

  void DialerImpl::newStream(const peer::PeerInfo &p,
                             const peer::Protocol &protocol,
                             StreamResultFunc cb) {
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
        });
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
