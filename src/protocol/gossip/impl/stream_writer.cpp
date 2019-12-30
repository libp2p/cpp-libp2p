/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/impl/stream_writer.hpp>

#include <cassert>

#include <libp2p/protocol/gossip/impl/message_builder.hpp>

namespace libp2p::protocol::gossip {

  StreamWriter::StreamWriter(const Config &config,
                             Scheduler &scheduler,
                             const Feedback &feedback,
                             std::shared_ptr<connection::Stream> stream,
                             PeerContextPtr peer)
      : timeout_(config.rw_timeout_msec),
        scheduler_(scheduler),
        feedback_(feedback),
        stream_(std::move(stream)),
        peer_(std::move(peer))
  // TODO(artem): issue with max message size and split
  {
    assert(feedback_);
    assert(stream_);
  }

  void StreamWriter::write(outcome::result<SharedBuffer> serialization_res) {
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

    auto& buffer = serialization_res.value();
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

  void StreamWriter::asyncPostError(Error error) {
    scheduler_.schedule([this, self_wptr = weak_from_this(), error] {
      if (self_wptr.expired() || closed_) {
        return;
      }
      feedback_(peer_, error);
    }).detach();
  }

  void StreamWriter::onMessageWritten(outcome::result<size_t> res) {
    if (writing_bytes_ == 0) {
      return;
    }

    if (!res) {
      feedback_(peer_, res.error());
      return;
    }
    if (writing_bytes_ != res.value()) {
      feedback_(peer_, Error::MESSAGE_WRITE_ERROR);
      return;
    }

    endWrite();

    if (!pending_buffers_.empty()) {
      SharedBuffer& buffer = pending_buffers_.front();
      pending_bytes_ -= buffer->size();
      beginWrite(std::move(buffer));
      pending_buffers_.pop_front();
    }
  }

  void StreamWriter::beginWrite(SharedBuffer buffer) {
    assert(buffer);

    auto data = buffer->data();
    writing_bytes_ = buffer->size();

    // clang-format off
    stream_->write(
        gsl::span(data, writing_bytes_),
        writing_bytes_,

        [self_wptr = weak_from_this(), this, buffer = std::move(buffer)]
            (outcome::result<size_t> result)
        {
          if (self_wptr.expired() || closed_) {
            return;
          }
          onMessageWritten(result);
        }
    );
    // clang-format on

    if (timeout_ > 0) {
      timeout_handle_ = scheduler_.schedule(
          timeout_,
          [self_wptr = weak_from_this(), this] {
            if (self_wptr.expired() || closed_) {
              return;
            }
            feedback_(peer_, Error::WRITER_TIMEOUT);
          });
    }
  }

  void StreamWriter::endWrite() {
    writing_bytes_ = 0;
    timeout_handle_.cancel();
  }

  void StreamWriter::close() {
    endWrite();
    closed_ = true;
    stream_->close([self{shared_from_this()}](outcome::result<void>) {});
  }

}  // namespace libp2p::protocol::gossip
