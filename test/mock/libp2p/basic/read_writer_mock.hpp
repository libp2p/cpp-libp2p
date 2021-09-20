/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_READ_WRITER_MOCK_HPP
#define LIBP2P_READ_WRITER_MOCK_HPP

#include <libp2p/basic/readwriter.hpp>

#include <gmock/gmock.h>

namespace libp2p::basic {
  struct ReadWriterMock : public ReadWriter {
    MOCK_METHOD3(read,
                 void(gsl::span<uint8_t>, size_t, Reader::ReadCallbackFunc));
    MOCK_METHOD3(readSome,
                 void(gsl::span<uint8_t>, size_t, Reader::ReadCallbackFunc));

    MOCK_METHOD3(write,
                 void(gsl::span<const uint8_t>, size_t,
                      Writer::WriteCallbackFunc));
    MOCK_METHOD3(writeSome,
                 void(gsl::span<const uint8_t>, size_t,
                      Writer::WriteCallbackFunc));
  };
}  // namespace libp2p::basic

#endif  // LIBP2P_READ_WRITER_MOCK_HPP
