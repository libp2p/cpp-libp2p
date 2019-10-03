/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/network/impl/transport_manager_impl.hpp"

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include "mock/libp2p/transport/transport_mock.hpp"

using namespace libp2p::network;
using namespace libp2p::multi;
using namespace libp2p::transport;
using namespace libp2p::common;

using testing::AtMost;
using testing::Return;

class TransportManagerTest : public testing::Test {
 protected:
  const Multiaddress kDefaultMultiaddress =
      "/ip4/192.168.0.1/tcp/228"_multiaddr;

  const std::shared_ptr<TransportMock> kTransport1 =
      std::make_shared<TransportMock>();
  const std::shared_ptr<TransportMock> kTransport2 =
      std::make_shared<TransportMock>();
  const std::vector<std::shared_ptr<TransportAdaptor>> kTransports{kTransport1,
                                                                   kTransport2};
};

#define ASSERT_SPAN_VECTOR_EQ(_span, vector) \
  ASSERT_EQ(_span, gsl::span<const std::shared_ptr<TransportAdaptor>>(vector));

/**
 * @given transport manager, created from the transports vector
 * @when getting transports, supported by the manager
 * @then response consists of transports from the initializer vector
 */
TEST_F(TransportManagerTest, CreateFromVector) {
  TransportManagerImpl manager{kTransports};

  ASSERT_SPAN_VECTOR_EQ(manager.getAll(), kTransports)
}

/**
 * @given transport manager with no supported transports
 * @when adding one transport to it
 * @then the transport is added
 */
TEST_F(TransportManagerTest, AddTransport) {
  TransportManagerImpl manager{{kTransport1}};
  auto transports = manager.getAll();
  ASSERT_EQ(transports.size(), 1);
  ASSERT_EQ(transports[0], kTransport1);
}

/**
 * @given transport manager with no supported transports
 * @when adding several transports to it at once
 * @then the transports are added
 */
TEST_F(TransportManagerTest, AddTransports) {
  TransportManagerImpl manager{kTransports};
  ASSERT_SPAN_VECTOR_EQ(manager.getAll(), kTransports)
}

/**
 * @given transport manager with several supported transports
 * @when clearing the manager
 * @then manager does not have supported transports
 */
TEST_F(TransportManagerTest, Clear) {
  TransportManagerImpl manager{kTransports};
  ASSERT_FALSE(manager.getAll().empty());

  manager.clear();
  ASSERT_TRUE(manager.getAll().empty());
}

/**
 * @given transport manager with several supported transports @and one of them
 * is able to dial with the given multiaddress
 * @when getting a best transport to dial with the given multiaddress
 * @then transport, which supports that multiaddress, is returned
 */
TEST_F(TransportManagerTest, FindBestSuccess) {
  TransportManagerImpl manager{kTransports};

  EXPECT_CALL(*kTransport1, canDial(kDefaultMultiaddress))
      .Times(AtMost(1))
      .WillOnce(Return(false));
  EXPECT_CALL(*kTransport2, canDial(kDefaultMultiaddress))
      .Times(AtMost(1))
      .WillOnce(Return(true));

  auto dialable_transport = manager.findBest(kDefaultMultiaddress);
  ASSERT_TRUE(dialable_transport);
  ASSERT_EQ(dialable_transport, kTransport2);
}

/**
 * @given transport manager with several supported transports @and several of
 * them are able to dial with the given multiaddress
 * @when getting a best transport to dial with the given multiaddress
 * @then one of the transports, which support that multiaddress, is returned
 */
TEST_F(TransportManagerTest, FindBestSeveralCanDial) {
  TransportManagerImpl manager{kTransports};

  EXPECT_CALL(*kTransport1, canDial(kDefaultMultiaddress))
      .Times(AtMost(1))
      .WillOnce(Return(true));
  EXPECT_CALL(*kTransport2, canDial(kDefaultMultiaddress))
      .Times(AtMost(1))
      .WillOnce(Return(true));

  auto dialable_transport = manager.findBest(kDefaultMultiaddress);
  ASSERT_TRUE(dialable_transport);
  ASSERT_TRUE(dialable_transport == kTransport1
              || dialable_transport == kTransport2);
}

/**
 * @given transport manager with several supported transports @and none of them
 * is able to dial with the given multiaddress
 * @when getting a best transport to dial with the given multiaddress
 * @then no transport is returned
 */
TEST_F(TransportManagerTest, FindBestFailure) {
  TransportManagerImpl manager{{kTransport1}};

  EXPECT_CALL(*kTransport1, canDial(kDefaultMultiaddress))
      .WillOnce(Return(false));

  auto dialable_transport = manager.findBest(kDefaultMultiaddress);
  ASSERT_FALSE(dialable_transport);
}
