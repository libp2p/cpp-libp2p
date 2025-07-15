/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/read.hpp>
#include <libp2p/basic/varint_reader.hpp>
#include <libp2p/common/outcome_macro.hpp>

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
    readVarint(std::move(conn),
               std::move(cb),
               0,
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

    read(conn,
         BytesOut{*varint_buf}.subspan(current_length, 1),
         [c = conn, cb = std::move(cb), current_length, varint_buf](
             outcome::result<void> result) mutable {
           IF_ERROR_CB_RETURN(result);

           auto varint_opt = multi::UVarint::create(
               BytesIn{*varint_buf}.first(current_length + 1));
           if (varint_opt) {
             return cb(*varint_opt);
           }

           readVarint(std::move(c),
                      std::move(cb),
                      current_length + 1,
                      std::move(varint_buf));
         });
  }
}  // namespace libp2p::basic
