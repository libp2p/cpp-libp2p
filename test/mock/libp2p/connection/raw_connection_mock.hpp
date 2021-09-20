/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_RAW_CONNECTION_MOCK_HPP
#define LIBP2P_RAW_CONNECTION_MOCK_HPP

#include <libp2p/connection/raw_connection.hpp>

#include <gmock/gmock.h>

namespace libp2p::connection {

  class RawConnectionMock : public virtual RawConnection {
   public:
    ~RawConnectionMock() override = default;

    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD0(close, outcome::result<void>());

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
    MOCK_METHOD2(deferReadCallback,
                 void(outcome::result<size_t>, Reader::ReadCallbackFunc));
    MOCK_METHOD2(deferWriteCallback,
                 void(std::error_code, Writer::WriteCallbackFunc));

    bool isInitiator() const noexcept override {
      return isInitiator_hack();
    }

    MOCK_CONST_METHOD0(isInitiator_hack, bool());

    MOCK_METHOD0(localMultiaddr, outcome::result<multi::Multiaddress>());

    MOCK_METHOD0(remoteMultiaddr, outcome::result<multi::Multiaddress>());
  };

}  // namespace libp2p::connection

#endif  // LIBP2P_RAW_CONNECTION_MOCK_HPP
