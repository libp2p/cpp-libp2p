/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/connection/stream.hpp>

namespace libp2p::transport {
  class QuicConnection;
}  // namespace libp2p::transport

namespace libp2p::transport::lsquic {
  struct StreamCtx;
}  // namespace libp2p::transport::lsquic

namespace libp2p::connection {
  class QuicStream : public Stream,
                     public std::enable_shared_from_this<QuicStream> {
   public:
    QuicStream(std::shared_ptr<transport::QuicConnection> conn,
               transport::lsquic::StreamCtx *stream_ctx,
               bool is_initiator);
    ~QuicStream() override;

    // clang-tidy cppcoreguidelines-special-member-functions
    QuicStream(const QuicStream &) = delete;
    void operator=(const QuicStream &) = delete;
    QuicStream(QuicStream &&) = delete;
    void operator=(QuicStream &&) = delete;

    // Reader
    void read(BytesOut out, size_t bytes, ReadCallbackFunc cb) override;
    void readSome(BytesOut out, size_t bytes, ReadCallbackFunc cb) override;
    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;

    // Writer
    void writeSome(BytesIn in, size_t bytes, WriteCallbackFunc cb) override;
    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override;

    // Stream
    bool isClosedForRead() const override;
    bool isClosedForWrite() const override;
    bool isClosed() const override;
    void close(VoidResultHandlerFunc cb) override;
    void reset() override;
    void adjustWindowSize(uint32_t new_size, VoidResultHandlerFunc cb) override;
    outcome::result<bool> isInitiator() const override;
    outcome::result<PeerId> remotePeerId() const override;
    outcome::result<Multiaddress> localMultiaddr() const override;
    outcome::result<Multiaddress> remoteMultiaddr() const override;

    void onClose();

   private:
    std::shared_ptr<transport::QuicConnection> conn_;
    transport::lsquic::StreamCtx *stream_ctx_;
    bool initiator_;
  };
}  // namespace libp2p::connection
