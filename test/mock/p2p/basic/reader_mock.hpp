/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READER_MOCK_HPP
#define KAGOME_READER_MOCK_HPP

#include <gmock/gmock.h>
#include "common/hexutil.hpp"
#include "p2p/basic/reader.hpp"

namespace libp2p::basic {
  class ReaderMock : public Reader {
   public:
    ~ReaderMock() override = default;

    MOCK_METHOD2(read, void(gsl::span<uint8_t>, Reader::ReadCallbackFunc));
    MOCK_METHOD2(readSome, void(gsl::span<uint8_t>, Reader::ReadCallbackFunc));
  };
}  // namespace libp2p::basic

inline std::ostream &operator<<(std::ostream &s,
                                const std::vector<unsigned char> &v) {
  s << kagome::common::hex_upper(v) << "\n";
  return s;
}

#endif  // KAGOME_READER_MOCK_HPP
