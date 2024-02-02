#include <libp2p/transport/quic/quic_connection.hpp>
#include <libp2p/transport/quic/quic_stream.hpp>

#include <iostream>

namespace libp2p::connection {

  QuicStream::QuicStream(libp2p::transport::QuicConnection &conn,
                         bool is_initiator)
      : m_is_initiator(is_initiator),
        m_stream(conn.m_conn),
        m_conn(conn)

  {}

  bool QuicStream::isClosedForRead() const {
    return !m_stream.is_open_read();
  }

  bool QuicStream::isClosedForWrite() const {
    return !m_stream.is_open_write();
  }

  bool QuicStream::isClosed() const {
    return !m_stream.is_open();
  }

  void QuicStream::close(Stream::VoidResultHandlerFunc cb) {
    m_stream.async_close(
        [this, cb{std::move(cb)}](boost::system::error_code ec) {
          if (ec) {
            SL_TRACE(log_, "stream close failed with {}", ec.message());
            cb(outcome::failure(ec));
          } else {
            cb(outcome::success());
            SL_TRACE(log_, "stream closed");
          }
        });
  }

  void QuicStream::reset() {}

  void QuicStream::adjustWindowSize(uint32_t new_size,
                                    Stream::VoidResultHandlerFunc cb) 
  {}

  outcome::result<bool> QuicStream::isInitiator() const {
    return m_is_initiator;
  }

  outcome::result<peer::PeerId> QuicStream::remotePeerId() const {
    return m_conn.remotePeer();
  }

  outcome::result<multi::Multiaddress> QuicStream::localMultiaddr() const {
    return m_conn.localMultiaddr();
  }

  outcome::result<multi::Multiaddress> QuicStream::remoteMultiaddr() const {
    return m_conn.remoteMultiaddr();
  }

  void QuicStream::read(BytesOut out,
                        size_t bytes,
                        basic::Reader::ReadCallbackFunc cb) {
    auto then = [this, handle{std::move(cb)}](boost::system::error_code ec,
                                              size_t bytes_read) {
      if (ec) {
        if (ec != nexus::quic::stream_error::eof) {
          SL_TRACE(log_, "async_read_some returned ", ec.message());
          handle(outcome::failure(ec));
        }
        return;
      }

      handle(bytes_read);
    };

    boost::asio::async_read(m_stream,
                            boost::asio::buffer(out, bytes),                           
                            boost::asio::transfer_all(), std::move(then));
  }

  void QuicStream::readSome(BytesOut out,
                            size_t bytes,
                            basic::Reader::ReadCallbackFunc cb) {
    m_stream.async_read_some(
        boost::asio::buffer(out, bytes),
        [this, handle{std::move(cb)}](boost::system::error_code ec,
                                      size_t bytes_read) {
          if (ec) {
            if (ec != nexus::quic::stream_error::eof) {
              SL_TRACE(log_, "async_read_some returned ", ec.message());
              handle(outcome::failure(ec));
            }
            return;
          }

          handle(bytes_read);
        });
  }

  void QuicStream::deferReadCallback(outcome::result<size_t> res,
                                     basic::Reader::ReadCallbackFunc cb) {}

  void QuicStream::write(BytesIn in,
                         size_t bytes,
                         basic::Writer::WriteCallbackFunc cb) {
    auto then = [this, handle{cb}](boost::system::error_code ec,
                                   size_t bytes_written) {
      if (ec) {
        SL_TRACE(log_, "async_write failed with ", ec.message());
        handle(outcome::failure(ec));
      } else {
        handle(bytes_written);
      }
    };
    // composed operation which repeatedly calls async_write_some() on m_stream
    // until done
    boost::asio::async_write(m_stream,
                             boost::asio::buffer(in.data(), bytes),                             
                             boost::asio::transfer_all(),
                             std::move(then));
  }

  void QuicStream::writeSome(BytesIn in,
                             size_t bytes,
                             basic::Writer::WriteCallbackFunc cb) {
    auto then = [this, handle{cb}](boost::system::error_code ec,
                                   size_t bytes_written) {
      if (ec) {
        SL_TRACE(log_, "async_write failed with ", ec.message());
        handle(outcome::failure(ec));
      } else {
        handle(bytes_written);
      }
    };
    /*
  boost::asio::async_write(m_stream, boost::asio::buffer( in.data(), bytes),
     std::move(then), boost::asio::transfer_at_least(1) );*/

    m_stream.async_write_some(boost::asio::buffer(in.data(), bytes),
                              std::move(then));
  }

  void QuicStream::deferWriteCallback(
      std::error_code ec,
      basic::Writer::WriteCallbackFunc
          cb)  // void(outcome::result<size_t> /*written bytes*/)
  {}

}  // namespace libp2p::connection