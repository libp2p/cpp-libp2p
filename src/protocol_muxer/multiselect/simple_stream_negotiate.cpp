/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/simple_stream_negotiate.hpp>

#include <libp2p/protocol_muxer/multiselect/serializing.hpp>
#include <libp2p/protocol_muxer/protocol_muxer.hpp>

namespace libp2p::protocol_muxer::multiselect {

  namespace {
    using Callback = std::function<void(outcome::result<void>)>;

    struct Buffers {
      MsgBuf written;
      MsgBuf read;
    };

    void completed(const Callback &cb, const Buffers &buffers) {
      // In this case we expect the exact echo in reply
      return (buffers.read == buffers.written)
          ? cb(outcome::success())
          : cb(ProtocolMuxer::Error::NEGOTIATION_FAILED);
    }

    void onLastBytesRead(const Callback &cb, const Buffers &buffers,
                         outcome::result<size_t> res) {
      if (!res) {
        return cb(res.error());
      }
      completed(cb, buffers);
    }

    void onFirstBytesRead(const std::shared_ptr<basic::ReadWriter> &stream,
                          std::function<void(outcome::result<void>)> cb,
                          std::shared_ptr<Buffers> buffers,
                          outcome::result<size_t> res) {
      if (!res) {
        return cb(res.error());
      }

      if (res.value() != kMaxVarintSize) {
        return cb(ProtocolMuxer::Error::INTERNAL_ERROR);
      }

      auto total_sz = buffers->written.size();
      if (total_sz == kMaxVarintSize) {
        // protocol_id consists of 1 byte, not standard but possible
        return completed(cb, *buffers);
      }

      assert(total_sz > kMaxVarintSize);

      size_t remaining_bytes = total_sz - kMaxVarintSize;

      gsl::span<uint8_t> span(buffers->read);
      span = span.subspan(kMaxVarintSize, remaining_bytes);

      stream->read(span, span.size(),
                   [stream = stream, cb = std::move(cb),
                    buffers = std::move(buffers)](outcome::result<size_t> res) {
                     onLastBytesRead(cb, *buffers, res);
                   });
    }

    void onPacketWritten(const std::shared_ptr<basic::ReadWriter> &stream,
                         std::function<void(outcome::result<void>)> cb,
                         std::shared_ptr<Buffers> buffers,
                         outcome::result<size_t> res) {
      if (!res) {
        return cb(res.error());
      }

      if (res.value() != buffers->written.size()) {
        return cb(ProtocolMuxer::Error::INTERNAL_ERROR);
      }

      gsl::span<uint8_t> span(buffers->read);
      span = span.first(kMaxVarintSize);

      stream->read(
          span, span.size(),
          [stream = stream, cb = std::move(cb),
           buffers = std::move(buffers)](outcome::result<size_t> res) mutable {
            onFirstBytesRead(stream, std::move(cb), std::move(buffers), res);
          });
    }
  }  // namespace

  void simpleStreamNegotiateImpl(
      const std::shared_ptr<basic::ReadWriter> &stream,
      const peer::Protocol &protocol_id,
      std::function<void(outcome::result<void>)> cb) {
    auto buffers = std::make_shared<Buffers>();
    buffers->written = detail::createMessage(protocol_id);
    buffers->read.resize(buffers->written.size());

    assert(buffers->written.size() >= kMaxVarintSize);

    gsl::span<const uint8_t> span(buffers->written);

    stream->write(
        span, span.size(),
        [stream = stream, cb = std::move(cb),
         buffers = std::move(buffers)](outcome::result<size_t> res) mutable {
          onPacketWritten(stream, std::move(cb), std::move(buffers), res);
        });
  }

}  // namespace libp2p::protocol_muxer::multiselect
