/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../../include/libp2p/basic/varint_reader.hpp"

#include <vector>

namespace {
  constexpr uint8_t kMaximumVarintLength = 9;  // taken from Go
}

namespace libp2p::basic {
  void VarintReader::readVarint(
      std::shared_ptr<ReadWriter> conn,
      std::function<void(boost::optional<multi::UVarint>)> cb) {
    readVarint(std::move(conn), std::move(cb), 0,
               std::make_shared<std::vector<uint8_t>>(kMaximumVarintLength, 0));
  }

  void VarintReader::readVarint(
      std::shared_ptr<ReadWriter> conn,
      std::function<void(boost::optional<multi::UVarint>)> cb,
      uint8_t current_length,
      std::shared_ptr<std::vector<uint8_t>> varint_buf) {
    if (current_length > kMaximumVarintLength) {
      // TODO(107): Reentrancy here, defer callback
      return cb(boost::none);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    conn->read(gsl::make_span(varint_buf->data() + current_length, 1), 1,
               [c = std::move(conn), cb = std::move(cb), current_length,
                varint_buf](auto &&res) mutable {
                 if (!res) {
                   return cb(boost::none);
                 }

                 auto varint_opt = multi::UVarint::create(
                     gsl::make_span(varint_buf->data(), current_length + 1));
                 if (varint_opt) {
                   return cb(*varint_opt);
                 }

                 readVarint(std::move(c), std::move(cb), ++current_length,
                            std::move(varint_buf));
               });
  }
}  // namespace libp2p::basic
