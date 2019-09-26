/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/injector/network_injector.hpp"

#include <gtest/gtest.h>
#include <set>

#include "mock/libp2p/muxer/muxer_adaptor_mock.hpp"
#include "mock/libp2p/security/security_adaptor_mock.hpp"
#include "mock/libp2p/transport/transport_mock.hpp"

using namespace libp2p;
using namespace network;
using namespace injector;
using namespace crypto;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

/**
 * @when make default injector
 * @then test compiles
 */
TEST(NetworkBuilder, DefaultBuilds) {
  auto injector = makeNetworkInjector();
  auto nw = injector.create<std::shared_ptr<Network> >();
  ASSERT_NE(nw, nullptr);

  auto idmgr = injector.create<std::shared_ptr<peer::IdentityManager> >();
  std::cout << "peer id " << idmgr->getId().toBase58() << "\n";
}

/**
 * @given a keypair
 * @when make default injector with this keypair
 * @then keypair is applied, network is created
 */
TEST(NetworkBuilder, CustomKeyPairBuilds) {
  KeyPair keyPair{
      {{Key::Type::ED25519, {1}}},
      {{Key::Type::ED25519, {2}}},
  };

  auto injector = makeNetworkInjector(useKeyPair(keyPair));

  auto nw = injector.create<std::shared_ptr<Network> >();
  ASSERT_NE(nw, nullptr);

  auto idmgr = injector.create<std::shared_ptr<peer::IdentityManager> >();
  ASSERT_NE(idmgr, nullptr);
  std::cout << "peer id " << idmgr->getId().toBase58() << std::endl;

  ASSERT_EQ(idmgr->getKeyPair(), keyPair);
}

/**
 * @given network injector with 2 sec, 4 mux, 5 transport adaptors
 * @when create network
 * @then correct number of instances created
 */
TEST(NetworkBuilder, CustomAdaptorsBuilds) {
  // clang-format off
  using namespace boost;

  // hack for nice mocks.
  struct SecMock : public NiceMock<security::SecurityAdaptorMock> {};
  struct MuxMock : public NiceMock<muxer::MuxerAdaptorMock> {};
  struct TrMock : public NiceMock<transport::TransportMock> {};

  auto injector = makeNetworkInjector(
      useSecurityAdaptors<
          security::Plaintext,
          SecMock
      >(),
      useMuxerAdaptors<
          muxer::Yamux,
          MuxMock
      >(),
      useTransportAdaptors<
          transport::TcpTransport,
          TrMock
      >()
  );

  // set expectations
  {
    {
      auto vec = injector.create<
          std::vector<std::shared_ptr<security::SecurityAdaptor> > >();
      EXPECT_EQ(vec.size(), 2);

      auto set =
          injector
              .create<std::set<std::shared_ptr<security::SecurityAdaptor> > >();
      EXPECT_EQ(set.size(), 2) << "number of unique instances is incorrect";
    }

    {
      auto vec =
          injector.create<std::vector<std::shared_ptr<muxer::MuxerAdaptor> > >();
      EXPECT_EQ(vec.size(), 2);

      auto set =
          injector.create<std::set<std::shared_ptr<muxer::MuxerAdaptor> > >();
      EXPECT_EQ(set.size(), 2) << "number of unique instances is incorrect";
    }

    {
      auto vec = injector.create<
          std::vector<std::shared_ptr<transport::TransportAdaptor> > >();
      EXPECT_EQ(vec.size(), 2);

      auto set =
          injector
              .create<std::set<std::shared_ptr<transport::TransportAdaptor> > >();
      EXPECT_EQ(set.size(), 2) << "number of unique instances is incorrect";
    }
  }

  // build network
  auto network = injector.create<std::shared_ptr<Network> >();
  ASSERT_NE(network, nullptr);

  // upgrader is not created above, because it is not required for any transport
  // so try to create upgrader to trigger expectations
  auto upgrader = injector.create<std::shared_ptr<transport::Upgrader> >();
  ASSERT_NE(upgrader, nullptr);
}
