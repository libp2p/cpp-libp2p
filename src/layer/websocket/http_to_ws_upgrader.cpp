/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <libp2p/layer/websocket/http_to_ws_upgrader.hpp>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/layer/websocket/ws_connection.hpp>
#include <libp2p/multi/multibase_codec/codecs/base64.hpp>
#include <libp2p/peer/peer_id.hpp>

#ifndef UNIQUE_NAME
#define UNIQUE_NAME(base) base##__LINE__
#endif  // UNIQUE_NAME

#define IO_OUTCOME_TRY_NAME(var, val, res, cb) \
  auto && (var) = (res);                       \
  if ((var).has_error()) {                     \
    cb((var).error());                         \
    return;                                    \
  }                                            \
  auto && (val) = (var).value();

#define IO_OUTCOME_TRY(name, res, cb) \
  IO_OUTCOME_TRY_NAME(UNIQUE_NAME(name), name, res, cb)

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::layer::websocket, HttpToWsUpgrader::Error,
                            e) {
  using E = libp2p::layer::websocket::HttpToWsUpgrader::Error;
  switch (e) {
    case E::BAD_REQUEST:
      return "Bad request";
    default:
      return "Unknown error";
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
      std::shared_ptr<const WsConnectionConfig> config,
      std::shared_ptr<crypto::hmac::HmacProvider> hmac_provider)
      : conn_{std::move(connection)},
        initiator_{is_initiator},
        connection_cb_{std::move(cb)},
        scheduler_{std::move(scheduler)},
        config_{std::move(config)},
        hmac_provider_{std::move(hmac_provider)},
        read_buffer_{std::make_shared<common::ByteArray>(kMaxMsgLen)},
        rw_{std::make_shared<HttpReadWriter>(conn_, read_buffer_)} {
    BOOST_ASSERT(conn_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(config_ != nullptr);
    BOOST_ASSERT(hmac_provider_ != nullptr);
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
          return self->onUpgraded(handle_result.error());
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
      return gsl::span<const uint8_t>(
          reinterpret_cast<const uint8_t *>(str.data()),  // NOLINT
          str.size());
    }
  }  // namespace

  gsl::span<const uint8_t> HttpToWsUpgrader::createHttpRequest() {
    auto request = strToSpan(
        "GET /index.html HTTP/1.1\r\n"  // FIXME hardcode
        "Host: www.example.com\r\n"
        "Connection: upgrade\r\n"
        "Upgrade: websocket\r\n"
        "\r\n");
    return request;
  }

  gsl::span<const uint8_t> HttpToWsUpgrader::createHttpResponse() {
    std::string r =
        "101 Switching Protocols\r\n"  // FIXME hardcode
        "Upgrade: websocket\r\n";
    if (key_.has_value()) {
      auto data = key_.value();
      auto guid = strToSpan("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
      data.insert(data.end(), guid.begin(), guid.end());
      auto k =
          common::unhex("55cd433be9568ee79525a0919cf4b31c28108cee").value();
      auto d = hmac_provider_->calculateDigest(crypto::hmac::HashType::SHA1, k,
                                               data);
      auto v = d.value();
      auto e = multi::detail::encodeBase64(v);
      r += "Sec-WebSocket-Accept: ";
      r += e;
      r += "\r\n";
    }
    r += "\r\n";

    auto response = strToSpan(r);
    return response;
  }

  void HttpToWsUpgrader::sendHttpUpgradeRequest(
      gsl::span<const uint8_t> payload, basic::Writer::WriteCallbackFunc cb) {
    // IO_OUTCOME_TRY(write_result, handshake_state_->writeMessage({}, payload),
    //                cb);
    // auto write_cb = [self{shared_from_this()}, cb{std::move(cb)},
    //                  wr{write_result}](outcome::result<size_t> result) {
    //   IO_OUTCOME_TRY(bytes_written, result, cb);
    //   if (wr.cs1 and wr.cs2) {
    //     self->setCipherStates(wr.cs1, wr.cs2);
    //   }
    //   cb(bytes_written);
    // };
    // rw_->write(write_result.data, write_cb);
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
      basic::MessageReadWriter::ReadCallbackFunc cb) {}

  //  namespace {
  //    gsl::span<const uint8_t> strToSpan(std::string_view str) {
  //      return gsl::span<const uint8_t>(
  //          reinterpret_cast<const uint8_t *>(str.data()),  // NOLINT
  //          str.size());
  //    }
  //  }  // namespace

  outcome::result<void> HttpToWsUpgrader::handleRemoteRequest(
      gsl::span<const uint8_t> payload) {
    bool method_is_get = false;
    bool connection_is_upgrade = false;
    bool upgrade_is_websocket = false;

    if (payload.subspan(0, 5) == strToSpan("GET /")) {  // NOLINT
      method_is_get = true;
    }

    auto delimiter = strToSpan("\r\n");

    ssize_t begin = 0;
    ssize_t end;
    for (ssize_t pos = 0; pos <= payload.size() - delimiter.size(); ++pos) {
      if (payload.subspan(pos, delimiter.size()) == delimiter) {
        end = pos;
        if (end == begin) {
          break;
        }
        auto line = payload.subspan(begin, end - begin);

        if (line.size() > 12
            and line.subspan(0, 12) == strToSpan("Connection: ")) {
          if (line.subspan(12) == strToSpan("Upgrade")) {
            connection_is_upgrade = true;
          }
        } else if (line.size() > 9
                   and line.subspan(0, 9) == strToSpan("Upgrade: ")) {
          if (line.subspan(9) == strToSpan("websocket")) {
            upgrade_is_websocket = true;
          }
        } else if (line.size() > 19
                   and line.subspan(0, 19)
                       == strToSpan("Sec-WebSocket-Key: ")) {
          std::string encoded_key(std::next(line.begin(), 19), line.end());
          OUTCOME_TRY(decoded_key, multi::detail::decodeBase64(encoded_key));
          key_.emplace(std::move(decoded_key));
        }

        begin = pos + delimiter.size();
        pos = begin;
      }
    }

    if (not method_is_get) {
      return Error::BAD_REQUEST;
    }
    if (not connection_is_upgrade) {
      return Error::BAD_REQUEST;
    }
    if (not upgrade_is_websocket) {
      return Error::BAD_REQUEST;
    }

    return outcome::success();
  }

  outcome::result<void> HttpToWsUpgrader::handleRemoteResponse(
      gsl::span<const uint8_t> payload) {
    // TODO process request:
    // - this is HTTP response with code 101
    // - header Upgrade is websocket
    // - Sec-WebSocket-Accept corresponds of request's Sec-WebSocket-Key

    return outcome::success();
  }

  void HttpToWsUpgrader::onUpgraded(outcome::result<void> result) {
    if (result.has_error()) {
      log_->error("handshake failed, {}", result.error().message());
      return connection_cb_(result.as_failure());
    }
    log_->info("WsHandshake succeeded");

    auto ws_connection =
        std::make_shared<connection::WsConnection>(config_, conn_, scheduler_);
    ws_connection->start();
    connection_cb_(std::move(ws_connection));
  }

}  // namespace libp2p::layer::websocket