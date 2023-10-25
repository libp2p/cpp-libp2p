/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_READER_MOCK_HPP
#define LIBP2P_READER_MOCK_HPP

#include <libp2p/basic/reader.hpp>

#include <gmock/gmock.h>

#include <libp2p/common/hexutil.hpp>

namespace libp2p::basic {
  class ReaderMock : public Reader {
   public:
    ~ReaderMock() override = default;

    MOCK_METHOD2(read, void(BytesOut, Reader::ReadCallbackFunc));
    MOCK_METHOD2(readSome, void(BytesOut, Reader::ReadCallbackFunc));
  };
}  // namespace libp2p::basic

inline std::ostream &operator<<(std::ostream &s,
                                const std::vector<unsigned char> &v) {
  s << common::hex_upper(v) << "\n";
  return s;
}

#endif  // LIBP2P_READER_MOCK_HPP
