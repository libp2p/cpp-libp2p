/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_READWRITECLOSER_MOCK_HPP
#define LIBP2P_READWRITECLOSER_MOCK_HPP

#include <libp2p/basic/readwritecloser.hpp>

#include <gmock/gmock.h>

#include <libp2p/common/hexutil.hpp>

namespace libp2p::basic {
  class ReadWriteCloserMock : public ReadWriteCloser {
   public:
    ~ReadWriteCloserMock() override = default;

    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD0(close, outcome::result<void>(void));

    MOCK_METHOD2(read, void(gsl::span<uint8_t>, Reader::ReadCallbackFunc));
    MOCK_METHOD2(readSome, void(gsl::span<uint8_t>, Reader::ReadCallbackFunc));
    MOCK_METHOD2(write,
                 void(gsl::span<const uint8_t>, Writer::WriteCallbackFunc));
    MOCK_METHOD2(writeSome,
                 void(gsl::span<const uint8_t>, Writer::WriteCallbackFunc));
  };
}  // namespace libp2p::basic

inline std::ostream &operator<<(std::ostream &s,
                                const std::vector<unsigned char> &v) {
  s << kagome::common::hex_upper(v) << "\n";
  return s;
}

#endif  // LIBP2P_READWRITECLOSER_MOCK_HPP
