/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stream.hpp"

#include <cassert>

#include <libp2p/basic/read.hpp>
#include <libp2p/basic/varint_reader.hpp>
#include <libp2p/basic/write.hpp>

#include "message_parser.hpp"
#include "peer_context.hpp"

#define TRACE_ENABLED 0
#include <libp2p/common/trace.hpp>

namespace libp2p::protocol::gossip {

  Stream::Stream(size_t stream_id,
                 const Config &config,
                 basic::Scheduler &scheduler,
                 const Feedback &feedback,
                 MessageReceiver &msg_receiver,
                 std::shared_ptr<connection::Stream> stream,
                 PeerContextPtr peer)
      : stream_id_(stream_id),
        timeout_(config.rw_timeout_msec),
        scheduler_(scheduler),
        max_message_size_(config.max_message_size),
        feedback_(feedback),
        msg_receiver_(msg_receiver),
        stream_(std::move(stream)),
        peer_(std::move(peer)),
        read_buffer_(std::make_shared<std::vector<uint8_t>>()) {
    assert(feedback_);
    assert(stream_);
  }

  void Stream::read() {
    if (stream_->isClosedForRead()) {
      asyncPostError(Error::READER_DISCONNECTED);
      return;
    }

    TRACE("reading length from {}:{}", peer_->str, stream_id_);

    libp2p::basic::VarintReader::readVarint(
        stream_,
        [self_wptr = weak_from_this(),
         this](outcome::result<multi::UVarint> varint) {
          if (self_wptr.expired()) {
            return;
          }
          onLengthRead(std::move(varint));
        });

    reading_ = true;
  }

  void Stream::onLengthRead(outcome::result<multi::UVarint> varint) {
    if (!reading_) {
      return;
    }
    if (varint.has_error()) {
      reading_ = false;
      feedback_(peer_, varint.error());
      return;
    }
    auto msg_len = varint.value().toUInt64();

    TRACE("reading {} bytes from {}:{}", msg_len, peer_->str, stream_id_);

    if (msg_len > max_message_size_) {
      feedback_(peer_, Error::MESSAGE_SIZE_ERROR);
      return;
    }

    read_buffer_->resize(msg_len);

    libp2p::read(stream_,
                 std::span(read_buffer_->data(), msg_len),
                 [self_wptr = weak_from_this(), this, buffer = read_buffer_](
                     outcome::result<void> res) {
                   if (self_wptr.expired()) {
                     return;
                   }
                   onMessageRead(std::forward<decltype(res)>(res));
                 });
  }

  void Stream::onMessageRead(outcome::result<void> res) {
    if (!reading_) {
      return;
    }

    reading_ = false;

    if (!res) {
      feedback_(peer_, res.error());
      return;
    }

    TRACE("read {} bytes from {}:{}",
          read_buffer_->size(),
          peer_->str,
          stream_id_);

    MessageParser parser;
    if (!parser.parse(*read_buffer_)) {
      feedback_(peer_, Error::MESSAGE_PARSE_ERROR);
      return;
    }

    parser.dispatch(peer_, msg_receiver_);

    // reads again
    read();
  }

  void Stream::write(outcome::result<SharedBuffer> serialization_res) {
    if (closed_) {
      return;
    }

    if (stream_->isClosedForWrite()) {
      asyncPostError(Error::WRITER_DISCONNECTED);
      return;
    }

    if (!serialization_res) {
      asyncPostError(Error::MESSAGE_SERIALIZE_ERROR);
      return;
    }

    auto &buffer = serialization_res.value();
    if (buffer->empty()) {
      return;
    }

    if (writing_bytes_ > 0) {
      pending_bytes_ += buffer->size();
      pending_buffers_.emplace_back(std::move(buffer));
    } else {
      beginWrite(std::move(buffer));
    }
  }

  void Stream::beginWrite(SharedBuffer buffer) {
    assert(buffer);

    writing_bytes_ = buffer->size();

    TRACE("writing {} bytes to {}:{}", writing_bytes_, peer_->str, stream_id_);

    libp2p::write(
        stream_,
        *buffer,
        [weak_self{weak_from_this()}, buffer](outcome::result<void> result) {
          auto self = weak_self.lock();
          if (not self) {
            return;
          }
          self->onMessageWritten(result);
        });

    if (timeout_ > std::chrono::milliseconds::zero()) {
      timeout_handle_ = scheduler_.scheduleWithHandle(
          [self_wptr = weak_from_this(), this] {
            if (self_wptr.expired() || closed_) {
              return;
            }
            feedback_(peer_, Error::WRITER_TIMEOUT);
          },
          timeout_);
    }
  }

  void Stream::onMessageWritten(outcome::result<void> res) {
    if (closed_) {
      return;
    }
    if (writing_bytes_ == 0) {
      return;
    }

    if (!res) {
      feedback_(peer_, res.error());
      return;
    }

    TRACE("written {} bytes to {}:{}", writing_bytes_, peer_->str, stream_id_);

    endWrite();

    if (!pending_buffers_.empty()) {
      SharedBuffer &buffer = pending_buffers_.front();
      pending_bytes_ -= buffer->size();
      beginWrite(std::move(buffer));
      pending_buffers_.pop_front();
    }
  }

  void Stream::asyncPostError(Error error) {
    scheduler_.schedule([this, self_wptr = weak_from_this(), error] {
      if (self_wptr.expired() || closed_) {
        return;
      }
      feedback_(peer_, error);
    });
  }

  void Stream::endWrite() {
    writing_bytes_ = 0;
    timeout_handle_.reset();
  }

  void Stream::close() {
    reading_ = false;
    endWrite();
    closed_ = true;
    stream_->reset();
  }

}  // namespace libp2p::protocol::gossip
