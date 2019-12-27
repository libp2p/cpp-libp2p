/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/impl/stream_reader.hpp>

#include <cassert>

#include <libp2p/basic/varint_reader.hpp>
#include <libp2p/protocol/gossip/impl/message_parser.hpp>

namespace libp2p::protocol::gossip {

  StreamReader::StreamReader(const Config &config,
                             Scheduler &scheduler,
                             const Feedback &feedback,
                             MessageReceiver &msg_receiver,
                             std::shared_ptr<connection::Stream> stream,
                             PeerContextPtr peer)
      : timeout_(config.rw_timeout_msec),
        scheduler_(scheduler),
        max_message_size_(config.max_message_size),
        feedback_(feedback),
        msg_receiver_(msg_receiver),
        stream_(std::move(stream)),
        peer_(std::move(peer)),
        buffer_(std::make_shared<std::vector<uint8_t>>())
  {
    assert(feedback_);
    assert(stream_);
  }

  void StreamReader::read() {
    if (stream_->isClosedForRead()) {
      feedback_(peer_, Error::READER_DISCONNECTED);
      return;
    }

    // clang-format off
    libp2p::basic::VarintReader::readVarint(
        stream_,
        [self_wptr = weak_from_this(), this]
        (boost::optional<multi::UVarint> varint_opt) {
          if (self_wptr.expired()) {
            return;
          }
          onLengthRead(std::move(varint_opt));
        }
    );
    // clang-format on

    beginRead();
  }

  void StreamReader::onLengthRead(boost::optional<multi::UVarint> varint_opt) {
    if (!reading_) {
      return;
    }
    if (!varint_opt) {
      endRead();
      feedback_(peer_, Error::READER_DISCONNECTED);
      return;
    }
    auto msg_len = varint_opt->toUInt64();

    if (msg_len > max_message_size_) {
      feedback_(peer_, Error::MESSAGE_SIZE_ERROR);
      return;
    }

    buffer_->resize(msg_len);

    // clang-format off
    stream_->read(
        gsl::span(buffer_->data(), msg_len),
        msg_len,
        [self_wptr = weak_from_this(), this, buffer = buffer_](auto &&res) {
          if (self_wptr.expired()) {
            return;
          }
          onMessageRead(std::forward<decltype(res)>(res));
        }
    );
    // clang-format on
  }

  void StreamReader::onMessageRead(outcome::result<size_t> res) {
    if (!reading_) {
      return;
    }

    endRead();

    if (!res) {
      feedback_(peer_, res.error());
      return;
    }
    if (buffer_->size() != res.value()) {
      feedback_(peer_, Error::MESSAGE_PARSE_ERROR);
      return;
    }

    MessageParser parser;
    if (!parser.parse(*buffer_)) {
      feedback_(peer_, Error::MESSAGE_PARSE_ERROR);
      return;
    }

    parser.dispatch(peer_, msg_receiver_);

    // reads again
    read();
  }

  void StreamReader::beginRead() {
    reading_ = true;
    if (timeout_ > 0) {
      timeout_handle_ = scheduler_.schedule(
          timeout_,
          [self_wptr = weak_from_this(), this] {
            if (self_wptr.expired()) {
              return;
            }
            feedback_(peer_, Error::READER_TIMEOUT);
          });
    }
  }

  void StreamReader::endRead() {
    reading_ = false;
    timeout_handle_.cancel();
  }

  void StreamReader::close() {
    endRead();
    stream_->close([self{shared_from_this()}](outcome::result<void>) {});
  }

}  // namespace libp2p::protocol::gossip
