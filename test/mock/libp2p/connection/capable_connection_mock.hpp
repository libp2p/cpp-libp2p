/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CAPABLE_CONNECTION_MOCK_HPP
#define LIBP2P_CAPABLE_CONNECTION_MOCK_HPP

#include <gmock/gmock.h>

#include <libp2p/connection/capable_connection.hpp>

namespace libp2p::connection {

  class CapableConnectionMock : public CapableConnection {
   public:
    ~CapableConnectionMock() override = default;

    MOCK_METHOD0(newStream, outcome::result<std::shared_ptr<Stream>>());

    MOCK_METHOD1(newStream, void(StreamHandlerFunc));

    MOCK_METHOD1(onStream, void(NewStreamHandlerFunc));

    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());

    MOCK_CONST_METHOD0(localPeer, outcome::result<peer::PeerId>());
    MOCK_CONST_METHOD0(remotePeer, outcome::result<peer::PeerId>());
    MOCK_CONST_METHOD0(remotePublicKey, outcome::result<crypto::PublicKey>());

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
      return true;  // TODO(artem): fix reuse connections in opposite direction
      // return isInitiator_hack();
    }
    MOCK_CONST_METHOD0(isInitiator_hack, bool());
    MOCK_METHOD0(localMultiaddr, outcome::result<multi::Multiaddress>());
    MOCK_METHOD0(remoteMultiaddr, outcome::result<multi::Multiaddress>());
  };

  class CapableConnBasedOnRawConnMock : public CapableConnection {
   public:
    explicit CapableConnBasedOnRawConnMock(std::shared_ptr<RawConnection> c)
        : real_(std::move(c)) {}

    ~CapableConnBasedOnRawConnMock() override = default;

    MOCK_METHOD0(newStream, outcome::result<std::shared_ptr<Stream>>());
    MOCK_METHOD1(newStream, void(StreamHandlerFunc));

    MOCK_METHOD1(onStream, void(NewStreamHandlerFunc));

    MOCK_METHOD0(start, void());

    MOCK_METHOD0(stop, void());

    MOCK_CONST_METHOD0(localPeer, outcome::result<peer::PeerId>());

    MOCK_CONST_METHOD0(remotePeer, outcome::result<peer::PeerId>());

    MOCK_CONST_METHOD0(remotePublicKey, outcome::result<crypto::PublicKey>());

    bool isInitiator() const noexcept override {
      return real_->isInitiator();
    }

    outcome::result<multi::Multiaddress> localMultiaddr() override {
      return real_->localMultiaddr();
    }

    outcome::result<multi::Multiaddress> remoteMultiaddr() override {
      return real_->remoteMultiaddr();
    };

    void read(gsl::span<uint8_t> in, size_t bytes,
              Reader::ReadCallbackFunc f) override {
      return real_->read(in, bytes, f);
    };

    void readSome(gsl::span<uint8_t> in, size_t bytes,
                  Reader::ReadCallbackFunc f) override {
      return real_->readSome(in, bytes, f);
    };

    void write(gsl::span<const uint8_t> in, size_t bytes,
               Writer::WriteCallbackFunc f) override {
      return real_->write(in, bytes, f);
    }

    void writeSome(gsl::span<const uint8_t> in, size_t bytes,
                   Writer::WriteCallbackFunc f) override {
      return real_->writeSome(in, bytes, f);
    }

    bool isClosed() const override {
      return real_->isClosed();
    }

    outcome::result<void> close() override {
      return real_->close();
    }

    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override {
      real_->deferReadCallback(res, std::move(cb));
    }

    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override {
      real_->deferWriteCallback(ec, std::move(cb));
    }

   private:
    std::shared_ptr<RawConnection> real_;
  };
}  // namespace libp2p::connection

#endif  // LIBP2P_CAPABLE_CONNECTION_MOCK_HPP
