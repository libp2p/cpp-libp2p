/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/network/impl/router_impl.hpp"

#include <gtest/gtest.h>
#include <gsl/span>
#include "mock/libp2p/connection/stream_mock.hpp"

using namespace libp2p::network;
using namespace libp2p::peer;
using namespace libp2p::connection;

class RouterTest : public ::testing::Test, public RouterImpl {
 public:
  static constexpr uint8_t kDefaultStreamId = 5;
  const std::shared_ptr<Stream> kStreamToSend =
      std::make_shared<StreamMock>(kDefaultStreamId);
  std::shared_ptr<Stream> stream_to_receive;

  const Protocol kDefaultProtocol = "/ping/1.5.2";
  const Protocol kVersionProtocolPrefix = "/ping/1.5";
  const Protocol kProtocolPrefix = "/ping/";

  const Protocol kAnotherProtocol = "/http/2.2.8";

  /**
   * Knowing that a provided stream is a mocked one, get its ID
   * @param stream - mock stream
   * @return id of the stream
   */
  uint8_t getStreamMockId(const Stream &stream) {
    return dynamic_cast<const StreamMock &>(stream).stream_id;
  }

  /**
   * Set a handler for a given protocol, which predicate will fail the test if
   * called
   * @param proto, for which the handler is to be set
   */
  void setHandlerWithFail(const Protocol &proto) {
    this->setProtocolHandler(proto, [](auto &&) { FAIL(); });
  }

  /**
   * Set handlers for the given protocols, which predicate will fail the test if
   * called
   * @param protocols, for which the handlers are to be set
   */
  void setHandlersWithFail(gsl::span<const Protocol> protocols) {
    for (const auto &proto : protocols) {
      setHandlerWithFail(proto);
    }
    ASSERT_TRUE(std::is_permutation(protocols.begin(), protocols.end(),
                                    this->getSupportedProtocols().begin()));
  }
};

/**
 * @given router @and protocol to be handled
 * @when setting a perfect-match handler for that protocol @and calling handle
 * @then the corresponding handler is invoked
 */
TEST_F(RouterTest, SetHandlerPerfect) {
  this->setProtocolHandler(kDefaultProtocol,
                           [this](std::shared_ptr<Stream> stream) mutable {
                             stream_to_receive = std::move(stream);
                           });

  EXPECT_TRUE(this->handle(kDefaultProtocol, kStreamToSend));
  EXPECT_TRUE(stream_to_receive);
  EXPECT_EQ(getStreamMockId(*kStreamToSend),
            getStreamMockId(*stream_to_receive));
}

/**
 * @given router @and protocol to be handled
 * @when setting a perfect-match handler for another protocol @and calling
 * handle
 * @then handle returns error
 */
TEST_F(RouterTest, SetHandlerPerfectInvokeFail) {
  setHandlerWithFail(kAnotherProtocol);

  EXPECT_FALSE(this->handle(kDefaultProtocol, kStreamToSend));
}

/**
 * @given router @and protocol to be handled
 * @when setting a set of predicate-match handlers for that protocol, one of
 * which matched the given protocol, @and calling handle
 * @then the corresponding handler is invoked
 */
TEST_F(RouterTest, SetHandlerWithPredicate) {
  // this match is shorter, than the next two; must not be invoked
  this->setProtocolHandler(kProtocolPrefix, [](auto &&) { FAIL(); },
                           [](auto &&) { return true; });

  // this match is equal to the next one, but its handler will evaluate to
  // false; must not be invoked
  this->setProtocolHandler(kVersionProtocolPrefix, [](auto &&) { FAIL(); },
                           [](auto &&) { return false; });

  // this match must be invoked
  this->setProtocolHandler(
      kVersionProtocolPrefix,
      [this](std::shared_ptr<Stream> stream) mutable {
        stream_to_receive = std::move(stream);
      },
      [this](const auto &proto) { return proto == kDefaultProtocol; });

  EXPECT_TRUE(this->handle(kDefaultProtocol, kStreamToSend));
  EXPECT_TRUE(stream_to_receive);
  EXPECT_EQ(getStreamMockId(*kStreamToSend),
            getStreamMockId(*stream_to_receive));
}

/**
 * @given router
 * @when setting protocol handlers
 * @then getSupportedProtocols() call returns protocol, which were set
 */
TEST_F(RouterTest, GetSupportedProtocols) {
  static const std::vector<Protocol> kExpectedVec1{kDefaultProtocol};
  static const std::vector<Protocol> kExpectedVec2{kDefaultProtocol,
                                                   kProtocolPrefix};

  ASSERT_TRUE(this->getSupportedProtocols().empty());

  setHandlerWithFail(kDefaultProtocol);
  ASSERT_EQ(this->getSupportedProtocols(), kExpectedVec1);

  setHandlerWithFail(kProtocolPrefix);
  // protocols may be returned in any order
  ASSERT_TRUE(std::is_permutation(kExpectedVec2.begin(), kExpectedVec2.end(),
                                  this->getSupportedProtocols().begin()));

  setHandlerWithFail(kDefaultProtocol);
  ASSERT_TRUE(std::is_permutation(kExpectedVec2.begin(), kExpectedVec2.end(),
                                  this->getSupportedProtocols().begin()));
}

/**
 * @given router with some protocols set
 * @when removing protocol handlers for a particular protocol
 * @then corresponding handlers are removed
 */
TEST_F(RouterTest, RemoveProtocolHandlers) {
  setHandlersWithFail(
      std::vector<Protocol>{kDefaultProtocol, kAnotherProtocol});

  this->removeProtocolHandlers(kAnotherProtocol);
  const auto supported_protos = this->getSupportedProtocols();
  ASSERT_EQ(supported_protos.size(), 1);
  ASSERT_EQ(supported_protos[0], kDefaultProtocol);
}

/**
 * @given router with some protocols set
 * @when removing protocol handlers for a particular prefix
 * @then corresponding handlers are removed
 */
TEST_F(RouterTest, RemoveProtocolHandlersForPrefix) {
  setHandlersWithFail(std::vector<Protocol>{
      kDefaultProtocol, kVersionProtocolPrefix, kAnotherProtocol});

  this->removeProtocolHandlers(kProtocolPrefix);
  const auto supported_protos = this->getSupportedProtocols();
  ASSERT_EQ(supported_protos.size(), 1);
  ASSERT_EQ(supported_protos[0], kAnotherProtocol);
}

/**
 * @given router with some protocols set
 * @when removing all protocol handlers
 * @then all handlers are removed
 */
TEST_F(RouterTest, RemoveAll) {
  setHandlersWithFail(
      std::vector<Protocol>{kDefaultProtocol, kAnotherProtocol});

  this->removeAll();
  ASSERT_TRUE(this->getSupportedProtocols().empty());
}
