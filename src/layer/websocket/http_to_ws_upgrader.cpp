/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/http_to_ws_upgrader.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <memory>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/crypto/sha/sha1.hpp>
#include <libp2p/layer/websocket/ws_connection.hpp>
#include <libp2p/multi/multibase_codec/codecs/base64.hpp>
#include <libp2p/transport/tcp/tcp_util.hpp>

#ifndef UNIQUE_NAME
#define UNIQUE_NAME(base) base##__LINE__
#endif  // UNIQUE_NAME

#define IO_OUTCOME_TRY_NAME(var, val, res, cb)    \
  auto && (var) = (res);                          \
  if ((var).has_error()) {                        \
    cb((var).error());                            \
    return;                                       \
  }                                               \
  [[maybe_unused]] auto && (val) = (var).value(); \
  void(0)

#define IO_OUTCOME_TRY(name, res, cb) \
  IO_OUTCOME_TRY_NAME(UNIQUE_NAME(name), name, res, cb)

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::layer::websocket, HttpToWsUpgrader::Error,
                            e) {
  using E = libp2p::layer::websocket::HttpToWsUpgrader::Error;
  switch (e) {
    case E::BAD_REQUEST_BAD_METHOD:
      return "Bad method of request";
    case E::BAD_REQUEST_BAD_UPDATE_HEADER:
      return "Update-header of request is absent or invalid";
    case E::BAD_REQUEST_BAD_CONNECTION_HEADER:
      return "Connection-header of request is absent or invalid";
    case E::BAD_REQUEST_BAD_WS_KEY_HEADER:
      return "SecWsKey-header of request is absent or invalid";
    case E::BAD_RESPONSE_BAD_STATUS:
      return "Bad status of response or invalid";
    case E::BAD_RESPONSE_BAD_UPDATE_HEADER:
      return "Update-header of response is absent or invalid";
    case E::BAD_RESPONSE_BAD_CONNECTION_HEADER:
      return "Connection-header of response is absent or invalid";
    case E::BAD_RESPONSE_BAD_WS_ACCEPT_HEADER:
      return "SecWsAccept-header of response is absent, invalid "
             "or does not match sent key";
    default:
      return "Unknown error (HttpToWsUpgrader::Error)";
  }
}

namespace libp2p::layer::websocket {

  namespace {
    template <typename T>
    void unused(T &&) {}
  }  // namespace

  HttpToWsUpgrader::HttpToWsUpgrader(
      std::shared_ptr<connection::LayerConnection> connection,
      bool is_initiator, LayerAdaptor::LayerConnCallbackFunc cb,
      std::shared_ptr<basic::Scheduler> scheduler,
      std::shared_ptr<crypto::random::RandomGenerator> random_generator,
      std::shared_ptr<const WsConnectionConfig> config)
      : conn_{std::move(connection)},
        initiator_{is_initiator},
        connection_cb_{std::move(cb)},
        scheduler_{std::move(scheduler)},
        random_generator_{std::move(random_generator)},
        config_{std::move(config)},
        read_buffer_{std::make_shared<common::ByteArray>(kMaxMsgLen)},
        rw_{std::make_shared<HttpReadWriter>(conn_, read_buffer_)} {
    BOOST_ASSERT(conn_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(random_generator_ != nullptr);
    BOOST_ASSERT(config_ != nullptr);
    read_buffer_->resize(kMaxMsgLen);
  }

  void HttpToWsUpgrader::upgrade() {
    auto result = run();
    if (result.has_error()) {
      connection_cb_(result.error());
    }
  }

  outcome::result<void> HttpToWsUpgrader::run() {
    std::vector<uint8_t> buffer(kMaxMsgLen);

    if (initiator_) {
      SL_TRACE(log_, "outgoing connection. stage 1: send request");
      auto request = createHttpRequest();
      sendHttpUpgradeRequest(request, [self{shared_from_this()}](auto result) {
        IO_OUTCOME_TRY(bytes_written, result, self->onUpgraded);
        if (0 == bytes_written) {
          return self->onUpgraded(std::errc::bad_message);
        }
        SL_TRACE(self->log_, "outgoing connection. stage 2: read response");
        self->readHttpUpgradeResponse([self](auto result) {
          IO_OUTCOME_TRY(bytes_read, result, self->onUpgraded);
          auto handle_result = self->handleRemoteResponse(*bytes_read);
          if (handle_result.has_error()) {
            // handle error if needed
          }
          return self->onUpgraded(handle_result);
        });
      });

    } else {
      SL_TRACE(log_, "incoming connection. stage 1: read request");
      readHttpUpgradeRequest([self{shared_from_this()}](auto result) {
        IO_OUTCOME_TRY(bytes_read, result, self->onUpgraded);

        auto handle_result = self->handleRemoteRequest(*bytes_read);
        SL_TRACE(self->log_, "incoming connection. stage 2: send response");

        if (handle_result.has_value()) {
          auto response = self->createHttpResponse();  // success
          self->sendHttpUpgradeResponse(response, [self](auto result) {
            IO_OUTCOME_TRY(bytes_read, result, self->onUpgraded);
            self->onUpgraded(outcome::success());
          });
        } else {
          auto response = self->createHttpResponse();  // error
          self->sendHttpUpgradeResponse(response, [self](auto) {
            SL_TRACE(self->log_, "incoming connection. stage 3: error sent");
          });
          self->onUpgraded(handle_result.as_failure());
        }
      });
    }

    return outcome::success();
  }

  namespace {
    gsl::span<const uint8_t> strToSpan(std::string_view str) {
      return {reinterpret_cast<const uint8_t *>(str.data()),  // NOLINT
              static_cast<gsl::span<const uint8_t>::index_type>(str.size())};
    }
  }  // namespace

  gsl::span<const uint8_t> HttpToWsUpgrader::createHttpRequest() {
    std::string host = "unknown";
    auto addr_res = conn_->remoteMultiaddr();
    if (addr_res.has_value()) {
      const auto &addr = addr_res.value();
      auto [h, p] = libp2p::transport::detail::getHostAndTcpPort(addr);
      host = fmt::format("{}:{}", h, p);
    }

    std::array<uint8_t, 16> data{};
    random_generator_->fillRandomly(data);
    key_ = multi::detail::encodeBase64({data.begin(), data.end()});

    request_ = fmt::format(
        "GET / HTTP/1.1\r\n"
        "Host: {}\r\n"
        "User-Agent: {}\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: {}\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n",
        host, kClientName, key_);

    return strToSpan(request_);
  }

  gsl::span<const uint8_t> HttpToWsUpgrader::createHttpResponse() {
    BOOST_ASSERT(not key_.empty());

    static const auto guid = strToSpan("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    std::vector<uint8_t> data(key_.begin(), key_.end());
    data.insert(data.end(), guid.begin(), guid.end());
    auto hash = crypto::sha1(data).value();
    auto encoded_hash = multi::detail::encodeBase64({hash.begin(), hash.end()});

    response_ = fmt::format(
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Server: {}\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "Sec-WebSocket-Accept: {}\r\n"
        "\r\n",
        kServerName, encoded_hash);

    return strToSpan(response_);
  }

  void HttpToWsUpgrader::sendHttpUpgradeRequest(
      gsl::span<const uint8_t> payload, basic::Writer::WriteCallbackFunc cb) {
    auto write_cb = [self{shared_from_this()}, cb{std::move(cb)}](auto result) {
      IO_OUTCOME_TRY(buffer, result, cb);
      cb(std::move(buffer));
    };
    rw_->write(payload, write_cb);
  }

  void HttpToWsUpgrader::readHttpUpgradeRequest(
      basic::MessageReadWriter::ReadCallbackFunc cb) {
    auto read_cb = [self{shared_from_this()}, cb{std::move(cb)}](auto result) {
      IO_OUTCOME_TRY(buffer, result, cb);
      cb(std::move(buffer));
    };
    rw_->read(read_cb);
  }

  void HttpToWsUpgrader::sendHttpUpgradeResponse(
      gsl::span<const uint8_t> payload, basic::Writer::WriteCallbackFunc cb) {
    auto write_cb = [self{shared_from_this()}, cb{std::move(cb)}](auto result) {
      IO_OUTCOME_TRY(buffer, result, cb);
      cb(std::move(buffer));
    };
    rw_->write(payload, write_cb);
  }

  void HttpToWsUpgrader::readHttpUpgradeResponse(
      basic::MessageReadWriter::ReadCallbackFunc cb) {
    auto read_cb = [self{shared_from_this()}, cb{std::move(cb)}](auto result) {
      IO_OUTCOME_TRY(buffer, result, cb);
      cb(std::move(buffer));
    };
    rw_->read(read_cb);
  }

  outcome::result<void> HttpToWsUpgrader::handleRemoteRequest(
      gsl::span<const uint8_t> payload) {
    bool method_is_get = false;
    bool connection_is_upgrade = false;
    bool upgrade_is_websocket = false;
    bool key_is_set = false;

    if (payload.subspan(0, 5) == strToSpan("GET /")) {  // NOLINT
      method_is_get = true;
    }

    auto delimiter = strToSpan("\r\n");

    ssize_t begin = 0;
    ssize_t end = 0;
    for (ssize_t pos = 0; pos <= payload.size() - delimiter.size(); ++pos) {
      if (payload.subspan(pos, delimiter.size()) == delimiter) {
        end = pos;
        if (end == begin) {
          break;
        }
        auto line = payload.subspan(begin, end - begin);

        if (line.size() > 12
            and boost::iequals(line.subspan(0, 12), "Connection: ")) {
          if (boost::iequals(line.subspan(12), "Upgrade")) {
            connection_is_upgrade = true;
          }
        } else if (line.size() > 9
                   and boost::iequals(line.subspan(0, 9), "Upgrade: ")) {
          if (boost::iequals(line.subspan(9), "websocket")) {
            upgrade_is_websocket = true;
          }
        } else if (line.size() > 19
                   and boost::iequals(line.subspan(0, 19),
                                      "Sec-WebSocket-Key: ")) {
          key_.assign(std::next(line.begin(), 19), line.end());
          key_is_set = true;
        }

        begin = pos + delimiter.size();
        pos = begin;
      }
    }

    if (not method_is_get) {
      return Error::BAD_REQUEST_BAD_METHOD;
    }
    if (not connection_is_upgrade) {
      return Error::BAD_REQUEST_BAD_CONNECTION_HEADER;
    }
    if (not upgrade_is_websocket) {
      return Error::BAD_REQUEST_BAD_UPDATE_HEADER;
    }
    if (not key_is_set) {
      return Error::BAD_REQUEST_BAD_WS_KEY_HEADER;
    }

    return outcome::success();
  }

  outcome::result<void> HttpToWsUpgrader::handleRemoteResponse(
      gsl::span<const uint8_t> payload) {
    bool status_is_101 = false;
    bool connection_is_upgrade = false;
    bool upgrade_is_websocket = false;
    std::string accept_hash;
    bool valid_accept = false;

    if (payload.subspan(0, 12) == strToSpan("HTTP/1.1 101")) {  // NOLINT
      status_is_101 = true;
    }

    auto delimiter = strToSpan("\r\n");

    ssize_t begin = 0;
    ssize_t end = 0;
    for (ssize_t pos = 0; pos <= payload.size() - delimiter.size(); ++pos) {
      if (payload.subspan(pos, delimiter.size()) == delimiter) {
        end = pos;
        if (end == begin) {
          break;
        }
        auto line = payload.subspan(begin, end - begin);

        if (line.size() > 12
            and boost::iequals(line.subspan(0, 12), "Connection: ")) {
          if (boost::iequals(line.subspan(12), "Upgrade")) {
            connection_is_upgrade = true;
          }
        } else if (line.size() > 9
                   and boost::iequals(line.subspan(0, 9), "Upgrade: ")) {
          if (boost::iequals(line.subspan(9), "websocket")) {
            upgrade_is_websocket = true;
          }
        } else if (line.size() > 22
                   and boost::iequals(line.subspan(0, 22),
                                      "Sec-WebSocket-Accept: ")) {
          accept_hash.assign(std::next(line.begin(), 22), line.end());
        }

        begin = pos + delimiter.size();
        pos = begin;
      }
    }

    BOOST_ASSERT(not key_.empty());
    static const auto guid = strToSpan("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    std::vector<uint8_t> data(key_.begin(), key_.end());
    data.insert(data.end(), guid.begin(), guid.end());
    auto hash = crypto::sha1(data).value();
    valid_accept =
        accept_hash == multi::detail::encodeBase64({hash.begin(), hash.end()});

    // - this is HTTP response with code 101
    if (not status_is_101) {
      return Error::BAD_RESPONSE_BAD_STATUS;
    }
    // - header Connection is upgrade
    if (not connection_is_upgrade) {
      return Error::BAD_RESPONSE_BAD_CONNECTION_HEADER;
    }
    // - header Upgrade is websocket
    if (not upgrade_is_websocket) {
      return Error::BAD_RESPONSE_BAD_UPDATE_HEADER;
    }
    // - Sec-WebSocket-Accept corresponds of request's Sec-WebSocket-Key
    if (not valid_accept) {
      return Error::BAD_RESPONSE_BAD_WS_ACCEPT_HEADER;
    }

    return outcome::success();
  }

  void HttpToWsUpgrader::onUpgraded(outcome::result<void> result) {
    if (result.has_error()) {
      log_->error("handshake failed, {}", result.error().message());
      return connection_cb_(result.as_failure());
    }
    log_->info("WsHandshake succeeded");

    auto ws_connection = std::make_shared<connection::WsConnection>(
        config_, conn_, scheduler_, rw_->remainingData());
    ws_connection->start();
    connection_cb_(std::move(ws_connection));
  }

}  // namespace libp2p::layer::websocket
