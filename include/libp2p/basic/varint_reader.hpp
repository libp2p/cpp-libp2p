/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_VARINT_READER_HPP
#define LIBP2P_VARINT_READER_HPP

#include <functional>
#include <memory>

#include <libp2p/basic/readwriter.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::basic {
  class VarintReader {
   public:
    enum class Error {
      NO_VARINT = 1,
    };

    /**
     * Read a varint from the connection
     * @param conn to be read from
     * @param cb to be called, when a varint or maximum bytes from the
     * connection are read
     */
    static void readVarint(
        std::shared_ptr<ReadWriter> conn,
        std::function<void(outcome::result<multi::UVarint>)> cb);

   private:
    static void readVarint(
        std::shared_ptr<ReadWriter> conn,
        std::function<void(outcome::result<multi::UVarint>)> cb,
        uint8_t current_length,
        std::shared_ptr<std::vector<uint8_t>> varint_buf);
  };
}  // namespace libp2p::basic

OUTCOME_HPP_DECLARE_ERROR(libp2p::basic, VarintReader::Error);

#endif  // LIBP2P_VARINT_READER_HPP
