#include <iostream>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/transport/impl/util.hpp>
#include <libp2p/transport/quic/quic_connection.hpp>
#include <libp2p/transport/quic/quic_listener.hpp>

namespace libp2p::transport {

  QuicListener::QuicListener(nexus::quic::server &s,
                             boost::asio::ssl::context &ssl,
                             TransportListener::HandlerFunc handler)
      : m_server(s),
        m_acceptor(s, ssl),
        log_(log::createLogger("QuicListener")) {}

  outcome::result<void> QuicListener::listen(
      const multi::Multiaddress &address) {
    if (!canListen(address)) {
      SL_TRACE(log_,
               "cannot listen on address: {} Address family not supported",
               address.getStringAddress());
      return std::errc::address_family_not_supported;
    }

    if (m_is_open) {
      return std::errc::already_connected;
    }
    m_is_open = true;
    
    m_listen_addr = address;

    try {
      OUTCOME_TRY(endpoint, detail::makeUdpEndpoint(address));

      SL_TRACE(log_, "listen on {}", address.getStringAddress());
      m_acceptor.listen(endpoint, m_incoming_conn_capacity);

      // start listening
      doAcceptConns();

      return outcome::success();
    } catch (const boost::system::system_error &e) {
      SL_ERROR(log_,
               "Cannot listen to {}: {}",
               address.getStringAddress(),
               e.code().message());
      return e.code();
    }
  }

  bool QuicListener::canListen(const multi::Multiaddress &ma) const {
    return detail::supportsIpQuic(ma);
  }

  outcome::result<multi::Multiaddress> QuicListener::getListenMultiaddr()
      const {
    if (m_listen_addr.has_value()) {
      return *m_listen_addr;
    } else {
      return outcome::failure(std::errc::invalid_argument);
    }
  }

  bool QuicListener::isClosed() const {
    return !m_acceptor.is_open();
  }

  outcome::result<void> QuicListener::close() {
    m_acceptor.close();
    return outcome::success();
  }

  void QuicListener::doAcceptConns() {
    auto conn = std::make_shared<QuicConnection>(m_acceptor);
    auto &c = conn->m_conn;
    m_acceptor.async_accept(c, [this, con{conn}](boost::system::error_code ec) {
      if (ec) {
        SL_TRACE(log_, "accept failed with {}  ->shutting down", ec.message());
        /// stop accepting new connections and streams entirely, and mark
        /// existing
        /// connections as 'going away'. each associated acceptor is responsible
        /// for
        /// closing its own socket
        m_server.close();
        return;
      }

      con->start();
      // start next accept
      doAcceptConns();
      SL_TRACE(log_, "new connection");      
      handle_(std::move(con));
    });
  }

}  // namespace libp2p::transport