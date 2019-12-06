/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/varint_reader.hpp>
#include <libp2p/protocol/kademlia/impl/kad_message.hpp>
#include <libp2p/protocol/kademlia/impl/kad_protocol_session.hpp>
#include <libp2p/protocol/kademlia/kad.hpp>

namespace libp2p::protocol::kademlia {

  KadProtocolSession::KadProtocolSession(
      std::weak_ptr<KadSessionHost> host,
      std::shared_ptr<connection::Stream> stream,
      scheduler::Ticks operations_timeout)
      : host_(std::move(host)), stream_(std::move(stream)),
      operations_timeout_(operations_timeout) {}

  bool KadProtocolSession::read() {
    if (reading_ || stream_->isClosedForRead() || host_.expired()) {
      return false;
    }
    reading_ = true;
    libp2p::basic::VarintReader::readVarint(
        stream_,
        [host_wptr = host_, self_wptr = weak_from_this(),
         this](boost::optional<multi::UVarint> varint_opt) {
          if (host_wptr.expired() || self_wptr.expired()) {
            return;
          }
          onLengthRead(std::move(varint_opt));
        });
    setTimeout();
    return true;
  }

  bool KadProtocolSession::write(const Message &msg) {
    auto buffer = std::make_shared<std::vector<uint8_t>>();
    if (!msg.serialize(*buffer)) {
      return false;
    }
    return write(std::move(buffer));
  }

  bool KadProtocolSession::write(Buffer buffer) {
    if (stream_->isClosedForWrite() || host_.expired()) {
      return false;
    }
    auto data = buffer->data();
    auto sz = buffer->size();
    stream_->write(
        gsl::span(data, sz), sz,
        [host_wptr = host_, self_wptr = weak_from_this(), this,
         buffer = std::move(buffer)](outcome::result<size_t> result) {
          if (host_wptr.expired() || self_wptr.expired()) {
            return;
          }
          onMessageWritten(result);
        });
    setTimeout();
    return true;
  }

  void KadProtocolSession::onLengthRead(
      boost::optional<multi::UVarint> varint_opt) {
    auto host = host_.lock();
    if (!host || state_ == CLOSED_STATE) {
      return;
    }
    if (!varint_opt) {
      cancelTimeout();
      host->onCompleted(stream_.get(), Error::MESSAGE_PARSE_ERROR);
      return;
    }
    auto msg_len = varint_opt->toUInt64();
    if (!buffer_) {
      buffer_ = std::make_shared<std::vector<uint8_t>>(msg_len, 0);
    } else {
      buffer_->resize(msg_len);
    }
    stream_->read(gsl::span(buffer_->data(), msg_len), msg_len,
                  [host_wptr = host_, self_wptr = weak_from_this(), this,
                   buffer = buffer_](auto &&res) {
                    if (host_wptr.expired() || self_wptr.expired()) {
                      return;
                    }
                    onMessageRead(std::forward<decltype(res)>(res));
                  });
  }

  void KadProtocolSession::onMessageRead(outcome::result<size_t> res) {
    cancelTimeout();
    auto host = host_.lock();
    if (!host || state_ == CLOSED_STATE) {
      return;
    }
    reading_ = false;
    if (!res) {
      host->onCompleted(stream_.get(), res.error());
      return;
    }
    if (buffer_->size() != res.value()) {
      host->onCompleted(stream_.get(), Error::MESSAGE_PARSE_ERROR);
      return;
    }
    Message msg;
    if (!msg.deserialize(buffer_->data(), buffer_->size())) {
      host->onCompleted(stream_.get(), Error::MESSAGE_PARSE_ERROR);
      return;
    }
    host->onMessage(stream_.get(), std::move(msg));
  }

  void KadProtocolSession::onMessageWritten(outcome::result<size_t> res) {
    cancelTimeout();
    auto host = host_.lock();
    if (!host || state_ == CLOSED_STATE) {
      return;
    }
    if (res) {
      host->onCompleted(stream_.get(), Error::SUCCESS);
    } else {
      host->onCompleted(stream_.get(), res.error());
    }
  }

  void KadProtocolSession::close() {
    state_ = CLOSED_STATE;
    cancelTimeout();
    stream_->close([self{shared_from_this()}](outcome::result<void>) {});
  }

  void KadProtocolSession::setTimeout() {
    if (operations_timeout_ == 0) {
      return;
    }
    auto host = host_.lock();
    if (!host) {
      return;
    }
    timeout_handle_ = host->scheduler().schedule(
      operations_timeout_,
      [host_wptr = host_, this] {
        if (host_wptr.expired()) {
          return;
        }
        host_wptr.lock()->onCompleted(stream_.get(), Error::TIMEOUT);
      });
  }

  void KadProtocolSession::cancelTimeout() {
    timeout_handle_.cancel();
  }

}  // namespace libp2p::protocol::kademlia
