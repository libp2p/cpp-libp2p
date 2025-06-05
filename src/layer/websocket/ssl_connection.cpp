/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/asio/use_awaitable.hpp>
#include <libp2p/layer/websocket/ssl_connection.hpp>

#include <libp2p/basic/read_return_size.hpp>
#include <libp2p/common/ambigous_size.hpp>
#include <libp2p/common/asio_buffer.hpp>
#include <libp2p/common/asio_cb.hpp>

namespace libp2p::connection {
  SslConnection::SslConnection(
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<LayerConnection> connection,
      std::shared_ptr<boost::asio::ssl::context> ssl_context)
      : connection_{std::move(connection)},
        ssl_context_{std::move(ssl_context)},
        ssl_{
            AsAsioReadWrite{std::move(io_context), connection_},
            *ssl_context_,
        } {}

  bool SslConnection::isInitiator() const {
    return connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> SslConnection::localMultiaddr() {
    return connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> SslConnection::remoteMultiaddr() {
    return connection_->remoteMultiaddr();
  }

  bool SslConnection::isClosed() const {
    return connection_->isClosed();
  }

  outcome::result<void> SslConnection::close() {
    return connection_->close();
  }

  void SslConnection::read(BytesOut out,
                           size_t bytes,
                           libp2p::basic::Reader::ReadCallbackFunc cb) {
    ambigousSize(out, bytes);
    readReturnSize(shared_from_this(), out, std::move(cb));
  }

  void SslConnection::readSome(BytesOut out,
                               size_t bytes,
                               libp2p::basic::Reader::ReadCallbackFunc cb) {
    ambigousSize(out, bytes);
    ssl_.async_read_some(asioBuffer(out), toAsioCbSize(std::move(cb)));
  }

  void SslConnection::writeSome(BytesIn in,
                                size_t bytes,
                                libp2p::basic::Writer::WriteCallbackFunc cb) {
    ambigousSize(in, bytes);
    ssl_.async_write_some(asioBuffer(in), toAsioCbSize(std::move(cb)));
  }

  void SslConnection::deferReadCallback(outcome::result<size_t> res,
                                        ReadCallbackFunc cb) {
    connection_->deferReadCallback(res, std::move(cb));
  }

  void SslConnection::deferWriteCallback(std::error_code ec,
                                         WriteCallbackFunc cb) {
    connection_->deferWriteCallback(ec, std::move(cb));
  }

  boost::asio::awaitable<outcome::result<size_t>> SslConnection::read(
      BytesOut out, size_t bytes) {
    ambigousSize(out, bytes);

    if (bytes == 0 || out.empty()) {
      co_return outcome::success(0);
    }

    size_t total_bytes_read = 0;

    while (total_bytes_read < bytes) {
      auto result = co_await readSome(BytesOut{out.data() + total_bytes_read,
                                               out.size() - total_bytes_read},
                                      bytes - total_bytes_read);

      if (!result) {
        co_return result.error();
      }

      size_t bytes_read = result.value();
      if (bytes_read == 0) {
        break;  // EOF reached
      }

      total_bytes_read += bytes_read;
    }

    co_return outcome::success(total_bytes_read);
  }

  boost::asio::awaitable<outcome::result<size_t>> SslConnection::readSome(
      BytesOut out, size_t bytes) {
    ambigousSize(out, bytes);

    if (bytes == 0 || out.empty()) {
      co_return outcome::success(0);
    }

    try {
      auto bytes_read = co_await ssl_.async_read_some(
          asioBuffer(out), boost::asio::use_awaitable);

      co_return outcome::success(bytes_read);
    } catch (const boost::system::system_error &e) {
      co_return outcome::failure(e.code());
    } catch (const std::exception &) {
      co_return outcome::failure(AsAsioReadWrite::error());
    }
  }

  boost::asio::awaitable<std::error_code> SslConnection::writeSome(
      BytesIn in, size_t bytes) {
    ambigousSize(in, bytes);

    if (bytes == 0 || in.empty()) {
      co_return std::error_code{};
    }

    try {
      co_await ssl_.async_write_some(asioBuffer(in),
                                     boost::asio::use_awaitable);

      co_return std::error_code{};
    } catch (const boost::system::system_error &e) {
      co_return e.code();
    } catch (const std::exception &) {
      co_return AsAsioReadWrite::error();
    }
  }
}  // namespace libp2p::connection
