/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONNECTION_LOOPBACKSTREAM
#define LIBP2P_CONNECTION_LOOPBACKSTREAM

#include <boost/asio.hpp>
#include <libp2p/connection/stream.hpp>

#include <libp2p/log/logger.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/peer/peer_info.hpp>

namespace libp2p::connection {

  class LoopbackStream : public libp2p::connection::Stream,
                         public std::enable_shared_from_this<LoopbackStream> {
   public:
    LoopbackStream(libp2p::peer::PeerInfo own_peer_info,
                   std::shared_ptr<boost::asio::io_context> io_context);

    bool isClosedForRead() const override;
    bool isClosedForWrite() const override;
    bool isClosed() const override;

    void close(VoidResultHandlerFunc cb) override;
    void reset() override;

    void adjustWindowSize(uint32_t new_size, VoidResultHandlerFunc cb) override;

    outcome::result<bool> isInitiator() const override;

    outcome::result<libp2p::peer::PeerId> remotePeerId() const override;

    outcome::result<libp2p::multi::Multiaddress> localMultiaddr()
        const override;

    outcome::result<libp2p::multi::Multiaddress> remoteMultiaddr()
        const override;

   protected:
    void read(gsl::span<uint8_t> out, size_t bytes,
              ReadCallbackFunc cb) override;

    void readSome(gsl::span<uint8_t> out, size_t bytes,
                  ReadCallbackFunc cb) override;

    void write(gsl::span<const uint8_t> in, size_t bytes,
               WriteCallbackFunc cb) override;

    void writeSome(gsl::span<const uint8_t> in, size_t bytes,
                   WriteCallbackFunc cb) override;

    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;

    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override;

   private:
    void read(gsl::span<uint8_t> out, size_t bytes, ReadCallbackFunc cb,
              bool some);

    libp2p::peer::PeerInfo own_peer_info_;
    std::shared_ptr<boost::asio::io_context> io_context_;

    log::Logger log_ = log::createLogger("LoopbackStream", "network");

    /// data, received for this stream, comes here
    boost::asio::streambuf buffer_;

    /// when a new data arrives, this function is to be called
    std::function<void(outcome::result<size_t>)> data_notifyee_;
    bool data_notified_ = false;

    /// is the stream opened for reads?
    bool is_readable_ = true;

    /// is the stream opened for writes?
    bool is_writable_ = true;

    /// was the stream reset?
    bool is_reset_ = false;
  };

}  // namespace libp2p::connection

#endif  // LIBP2P_CONNECTION_LOOPBACKSTREAM
