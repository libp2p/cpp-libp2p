/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/injector/host_injector.hpp"

#include <gtest/gtest.h>
#include "mock/libp2p/muxer/muxer_adaptor_mock.hpp"
#include "mock/libp2p/security/security_adaptor_mock.hpp"
#include "mock/libp2p/transport/transport_mock.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace libp2p;
using namespace injector;

using ::testing::NiceMock;

/**
 * @given default host injector
 * @when create sptr<Host> and uptr<Host>
 * @then valid instance is created
 */
TEST(HostInjector, Default) {
  testutil::prepareLoggers();

  auto injector = makeHostInjector();

  auto unique = injector.create<std::unique_ptr<Host>>();
  auto shared = injector.create<std::shared_ptr<Host>>();

  ASSERT_NE(unique, nullptr);
  ASSERT_NE(shared, nullptr);
}

/**
 * @given Host injector with overrided adaptors
 * @when use 2 adaptors of each type
 * @then Host has 2 adaptors of each type
 */
TEST(HostInjector, CustomAdaptors) {
  testutil::prepareLoggers();

  // hack for nice mocks.
  struct SecMock : public NiceMock<security::SecurityAdaptorMock> {};
  struct MuxMock : public NiceMock<muxer::MuxerAdaptorMock> {};
  struct TrMock : public NiceMock<transport::TransportMock> {};

  // clang-format off
  auto injector = makeHostInjector(
      useSecurityAdaptors<security::Plaintext, SecMock>(),
      useMuxerAdaptors<muxer::Yamux, MuxMock>(),
      useTransportAdaptors<transport::TcpTransport, TrMock>()
  );

  // set expectations
  {
    {
      auto vec = injector.create<
          std::vector<std::shared_ptr<security::SecurityAdaptor>>>();
      EXPECT_EQ(vec.size(), 2);

      auto set =
          injector
              .create<std::set<std::shared_ptr<security::SecurityAdaptor>>>();
      EXPECT_EQ(set.size(), 2) << "number of unique instances is incorrect";
    }

    {
      auto vec =
          injector.create<std::vector<std::shared_ptr<muxer::MuxerAdaptor>>>();
      EXPECT_EQ(vec.size(), 2);

      auto set =
          injector.create<std::set<std::shared_ptr<muxer::MuxerAdaptor>>>();
      EXPECT_EQ(set.size(), 2) << "number of unique instances is incorrect";
    }

    {
      auto vec = injector.create<
          std::vector<std::shared_ptr<transport::TransportAdaptor>>>();
      EXPECT_EQ(vec.size(), 2);

      auto set =
          injector
              .create<std::set<std::shared_ptr<transport::TransportAdaptor>>>();
      EXPECT_EQ(set.size(), 2) << "number of unique instances is incorrect";
    }
  }

  // build host
  auto host = injector.create<std::shared_ptr<Host>>();
  ASSERT_NE(host, nullptr);

  // upgrader is not created above, because it is not required for any transport
  // so try to create upgrader to trigger expectations
  auto upgrader = injector.create<std::shared_ptr<transport::Upgrader>>();
  ASSERT_NE(upgrader, nullptr);
}
