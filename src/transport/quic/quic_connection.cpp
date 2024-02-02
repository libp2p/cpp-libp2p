#include <libp2p/transport/quic/quic_connection.hpp>
#include <libp2p/transport/quic/quic_stream.hpp>

#include <libp2p/outcome/outcome.hpp>
#include <libp2p/transport/impl/util.hpp>

#include <libp2p/peer/peer_id.hpp>

namespace libp2p::transport {

  namespace {

    inline std::error_code convert(boost::system::errc::errc_t ec) {
      return {static_cast<int>(ec), std::system_category()};
    }

    inline std::error_code convert(std::error_code ec) {
      return ec;
    }
  }  // namespace

  QuicConnection::QuicConnection(nexus::quic::client &c)
      : m_is_initiator(true),
        m_conn(c),
        log_(log::createLogger("QuicConnection")) {}

  QuicConnection::QuicConnection(nexus::quic::acceptor &a)
      : m_conn(a), log_(log::createLogger("QuicConnection")) {}

  QuicConnection::QuicConnection(nexus::quic::client &c,
                                 const Udp::endpoint &endpoint,
                                 const std::string_view &hostname)
      : m_is_initiator(true),
        m_conn(c, endpoint, hostname.data()),
        log_(log::createLogger("QuicConnection")) {}

  void QuicConnection::accept_streams() {
    auto instream = std::make_shared<connection::QuicStream>(*this, false);

    m_conn.async_accept(
        instream->m_stream,
        [this, s{instream}](boost::system::error_code ec) mutable {
          if (ec) {
            SL_TRACE(log_, "stream accept failed with {}", ec.message());
            m_on_stream_cb(
                nullptr);  // change signature of callback to propagate error
            return;
          }
          // start next accept
          accept_streams();
          // start reading from stream
          SL_TRACE(log_, "new stream");

          m_on_stream_cb(std::move(
              s));  // what if QuicConnection::onStream( on_stream_handler ) has
                    // not been called yet ?! can this happen
        });
  }

  /**
   * @brief Function that is used to check if current object is closed.
   * @return true if closed, false otherwise
   */
  bool QuicConnection::isClosed() const {
    return !m_conn.is_open();
  }

  /**
   * @brief Closes current object
   * @return nothing or error
   */
  outcome::result<void> QuicConnection::close() {
    boost::system::error_code ec;
    m_conn.close(ec);

    if (ec) {
      return outcome::failure(ec);
    } else {
      return outcome::success();
    }
  }

  /*start accepting incoming streams*/
  void QuicConnection::start() {
    SL_TRACE(log_, "QuicConnection started");
    accept_streams();
  }

  void QuicConnection::stop() {
    // nothing to do ?! accept_stream loop will cancel itself once conn gets
    // closed
  }

  void QuicConnection::newStream(CapableConnection::StreamHandlerFunc cp) {
    auto outstream = std::make_shared<connection::QuicStream>(*this, true);

    m_conn.async_connect(
        outstream->m_stream,
        [out_stream = std::move(outstream), cp{std::move(cp)}, this](
            boost::system::error_code ec) {
          if (ec) {
            SL_TRACE(log_, "async_connect failed with {}", ec.message());
            cp(outcome::failure(ec));
            return;
          }
          SL_TRACE(log_, "async_connect stream successful");
          cp(std::move(out_stream));
        });
  }

  outcome::result<std::shared_ptr<libp2p::connection::Stream>>
  QuicConnection::newStream() {
    auto outstream = std::make_shared<connection::QuicStream>(*this, true);
    boost::system::error_code ec;
    m_conn.connect(outstream->m_stream, ec);

    if (ec) {
      return outcome::failure(ec);
    } else {
      return outstream;
    }
  }

  void QuicConnection::onStream(NewStreamHandlerFunc cb) {
    m_on_stream_cb = std::move(cb);
  }

  /**
   * Get a public key of peer this connection is established with
   * @return public key
   */
  outcome::result<crypto::PublicKey> QuicConnection::remotePublicKey() const {
    return {{}};
  }

  outcome::result<multi::Multiaddress> QuicConnection::remoteMultiaddr() {
    if (!remote_multiaddress_) {
      auto res = saveMultiaddresses();
      if (!res) {
        return res.error();
      }
    }
    return remote_multiaddress_.value();
  }

  outcome::result<multi::Multiaddress> QuicConnection::localMultiaddr() {
    if (!local_multiaddress_) {
      auto res = saveMultiaddresses();
      if (!res) {
        return res.error();
      }
    }
    return local_multiaddress_.value();
    return *local_multiaddress_;
  }

  /**
   * Get a PeerId of our local peer
   * @return peer id
   */
  outcome::result<peer::PeerId> QuicConnection::localPeer() const {
    if (auto addr = const_cast<QuicConnection *>(this)->localMultiaddr();
        addr) {
      if (auto val = local_multiaddress_.value().getPeerId(); val) {
        return peer::PeerId::fromBase58(val.value());
      } else {
        return outcome::failure(std::errc::invalid_argument);
      }
    } else {
      return outcome::failure(std::errc::invalid_argument);
    }
  }

  /**
   * Get a PeerId of peer this connection is established with
   * @return peer id
   */
  outcome::result<peer::PeerId> QuicConnection::remotePeer() const {
    if (auto addr = const_cast<QuicConnection *>(this)->remoteMultiaddr();
        addr) {
      if (auto val = remote_multiaddress_.value().getPeerId(); val) {
        return peer::PeerId::fromBase58(val.value());
      } else {
        return outcome::failure(std::errc::invalid_argument);
      }
    } else {
      return outcome::failure(addr.error());
    }
  }

  bool QuicConnection::isInitiator() const noexcept {
    return m_is_initiator;
  }

  void QuicConnection::setRemoteEndpoint(const multi::Multiaddress &remote) {
    remote_multiaddress_ = remote;
  }

  outcome::result<void> QuicConnection::saveMultiaddresses() {
    boost::system::error_code ec;
    if (m_conn.is_open()) {
      if (!local_multiaddress_) {
        auto endpoint(m_conn.local_endpoint());
        if (!ec) {
          OUTCOME_TRY(addr, detail::makeAddress(endpoint));
          local_multiaddress_ = std::move(addr);
        }
      }
      if (!remote_multiaddress_) {
        auto endpoint(m_conn.remote_endpoint(ec));
        if (!ec) {
          OUTCOME_TRY(addr, detail::makeAddress(endpoint));
          remote_multiaddress_ = std::move(addr);
        }
      }
    } else {
      if (remote_multiaddress_)  // maybe it was set by the QuicTransport with
                                 // setRemote()
      {
        return outcome::success();
      } else {
        return convert(boost::system::errc::not_connected);
      }
    }
    if (ec) {
      return convert(ec);
    }
#ifndef NDEBUG
    debug_str_ = fmt::format("{} {} {}",
                             local_multiaddress_->getStringAddress(),
                             m_is_initiator ? "->" : "<-",
                             remote_multiaddress_->getStringAddress());
#endif
    return outcome::success();
  }

  /*
      void QuicConnection::deferReadCallback(outcome::result<size_t> res,
                             ReadCallbackFunc cb)
      {

      }

      void QuicConnection::deferWriteCallback(std::error_code ec,
     WriteCallbackFunc cb)
      {

      }

  */

}  // namespace libp2p::transport