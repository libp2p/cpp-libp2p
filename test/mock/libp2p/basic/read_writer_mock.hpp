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
                 void(BytesOut, size_t, Reader::ReadCallbackFunc));
    MOCK_METHOD3(readSome,
                 void(BytesOut, size_t, Reader::ReadCallbackFunc));

    MOCK_METHOD3(write,
                 void(BytesIn, size_t,
                      Writer::WriteCallbackFunc));
    MOCK_METHOD3(writeSome,
                 void(BytesIn, size_t,
                      Writer::WriteCallbackFunc));
  };
}  // namespace libp2p::basic

#endif  // LIBP2P_READ_WRITER_MOCK_HPP
