/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/basic/varint_reader.hpp"

#include <vector>

namespace {
  constexpr uint8_t kMaximumVarintLength = 9;  // taken from Go
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::basic, VarintReader::Error, e) {
  using E = libp2p::basic::VarintReader::Error;
  switch (e) {
    case E::NO_VARINT:
      return "Input stream does not contain a varint value";
  }
  return "Unknown error";
}

namespace libp2p::basic {
  void VarintReader::readVarint(
      std::shared_ptr<ReadWriter> conn,
      std::function<void(outcome::result<multi::UVarint>)> cb) {
    readVarint(std::move(conn), std::move(cb), 0,
               std::make_shared<std::vector<uint8_t>>(kMaximumVarintLength, 0));
  }

  void VarintReader::readVarint(
      std::shared_ptr<ReadWriter> conn,
      std::function<void(outcome::result<multi::UVarint>)> cb,
      uint8_t current_length,
      std::shared_ptr<std::vector<uint8_t>> varint_buf) {
    if (current_length > kMaximumVarintLength) {
      // TODO(107): Reentrancy here, defer callback
      // to the moment we read more bytes than varint may contain and still no
      // valid varint was parsed
      return cb(Error::NO_VARINT);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    conn->read(gsl::make_span(varint_buf->data() + current_length, 1), 1,
               [c = std::move(conn), cb = std::move(cb), current_length,
                varint_buf](auto &&res) mutable {
                 if (not res.has_value()) {
                   return cb(res.error());
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
