/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECURE_CONNECTION_MOCK_HPP
#define LIBP2P_SECURE_CONNECTION_MOCK_HPP

#include <libp2p/connection/secure_connection.hpp>

#include <gmock/gmock.h>

namespace libp2p::connection {
  class SecureConnectionMock : public SecureConnection {
   public:
    ~SecureConnectionMock() override = default;

    MOCK_CONST_METHOD0(isClosed, bool(void));

    MOCK_METHOD0(close, outcome::result<void>(void));

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

    MOCK_CONST_METHOD0(isInitiatorMock, bool(void));
    bool isInitiator() const noexcept override {
      return isInitiatorMock();
    }

    MOCK_METHOD0(localMultiaddr, outcome::result<multi::Multiaddress>(void));

    MOCK_METHOD0(remoteMultiaddr, outcome::result<multi::Multiaddress>(void));

    MOCK_CONST_METHOD0(localPeer, outcome::result<peer::PeerId>(void));

    MOCK_CONST_METHOD0(remotePeer, outcome::result<peer::PeerId>(void));

    MOCK_CONST_METHOD0(remotePublicKey,
                       outcome::result<crypto::PublicKey>(void));
  };
}  // namespace libp2p::connection

#endif  // LIBP2P_SECURE_CONNECTION_MOCK_HPP
