/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_INSECURE_RW_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_INSECURE_RW_HPP

#include <memory>

#include <libp2p/basic/message_read_writer.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/connection/raw_connection.hpp>

namespace libp2p::security::noise {

  /**
   * Does raw messages exchange, primary during handshake.
   * Implements transparent messages prefixing with length marker (16bytes big
   * endian prefix).
   *
   * Does __NOT__ destroy connection during destruction and buffer is not going
   * to be destroyed too.
   */
  class InsecureReadWriter
      : public basic::MessageReadWriter,
        public std::enable_shared_from_this<InsecureReadWriter> {
   public:
    /**
     * Initializes read writer
     * @param connection - raw connection
     * @param buffer - pointer to buffer where to store bytes from network
     */
    InsecureReadWriter(std::shared_ptr<connection::LayerConnection> connection,
                       std::shared_ptr<common::ByteArray> buffer);

    /// read next message from the network
    void read(ReadCallbackFunc cb) override;

    /// write the given bytes to the network
    void write(gsl::span<const uint8_t> buffer,
               basic::Writer::WriteCallbackFunc cb) override;

   private:
    std::shared_ptr<connection::LayerConnection> connection_;
    std::shared_ptr<common::ByteArray> buffer_;
    common::ByteArray outbuf_;
  };

}  // namespace libp2p::security::noise

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_INSECURE_RW_HPP
