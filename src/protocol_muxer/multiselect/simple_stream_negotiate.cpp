/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect/simple_stream_negotiate.hpp>

#include <libp2p/protocol_muxer/multiselect/serializing.hpp>
#include <libp2p/protocol_muxer/protocol_muxer.hpp>

namespace libp2p::protocol_muxer::multiselect {

  namespace {
    using StreamPtr = std::shared_ptr<connection::Stream>;
    using Callback = std::function<void(outcome::result<StreamPtr>)>;

    struct Buffers {
      MsgBuf written;
      MsgBuf read;
    };

    void failed(const StreamPtr &stream, const Callback &cb,
                std::error_code ec) {
      stream->reset();
      cb(ec);
    }

    void completed(StreamPtr stream, const Callback &cb,
                   const Buffers &buffers) {
      // In this case we expect the exact echo in reply
      if (buffers.read == buffers.written) {
        return cb(std::move(stream));
      }
      failed(stream, cb, ProtocolMuxer::Error::NEGOTIATION_FAILED);
    }

    void onLastBytesRead(StreamPtr stream, const Callback &cb,
                         const Buffers &buffers, outcome::result<size_t> res) {
      if (!res) {
        return failed(stream, cb, res.error());
      }
      completed(std::move(stream), cb, buffers);
    }

    void onFirstBytesRead(StreamPtr stream, Callback cb,
                          std::shared_ptr<Buffers> buffers,
                          outcome::result<size_t> res) {
      if (!res) {
        return failed(stream, cb, res.error());
      }

      if (res.value() != kMaxVarintSize) {
        return failed(stream, cb, ProtocolMuxer::Error::INTERNAL_ERROR);
      }

      auto total_sz = buffers->written.size();
      if (total_sz == kMaxVarintSize) {
        // protocol_id consists of 1 byte, not standard but possible
        return completed(std::move(stream), cb, *buffers);
      }

      assert(total_sz > kMaxVarintSize);

      size_t remaining_bytes = total_sz - kMaxVarintSize;

      gsl::span<uint8_t> span(buffers->read);
      span = span.subspan(kMaxVarintSize, remaining_bytes);

      stream->read(
          span, span.size(),
          [stream = stream, cb = std::move(cb),
           buffers = std::move(buffers)](outcome::result<size_t> res) mutable {
            onLastBytesRead(std::move(stream), cb, *buffers, res);
          });
    }

    void onPacketWritten(StreamPtr stream, Callback cb,
                         std::shared_ptr<Buffers> buffers,
                         outcome::result<size_t> res) {
      if (!res) {
        return failed(stream, cb, res.error());
      }

      if (res.value() != buffers->written.size()) {
        return failed(stream, cb, ProtocolMuxer::Error::INTERNAL_ERROR);
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

  void simpleStreamNegotiate(const StreamPtr &stream,
                             const peer::Protocol &protocol_id, Callback cb) {
    auto buffers = std::make_shared<Buffers>();
    buffers->written = detail::createMessage(protocol_id);
    buffers->read.resize(buffers->written.size());

    assert(buffers->written.size() >= kMaxVarintSize);

    gsl::span<const uint8_t> span(buffers->written);

    stream->write(
        span, span.size(),
        [stream = stream, cb = std::move(cb),
         buffers = std::move(buffers)](outcome::result<size_t> res) mutable {
          onPacketWritten(std::move(stream), std::move(cb), std::move(buffers), res);
        });
  }

}  // namespace libp2p::protocol_muxer::multiselect
