/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/common/metrics/instance_list.hpp>

#include <gtest/gtest.h>
#include <set>

namespace libp2p {
  struct Foo {
    int bar{};

    LIBP2P_METRICS_INSTANCE_COUNT(libp2p::Foo);
    LIBP2P_METRICS_INSTANCE_LIST(libp2p::Foo);
  };
}  // namespace libp2p

using libp2p::Foo;

void expectFoo(const std::set<const Foo *> &expected) {
  EXPECT_EQ(Foo::Libp2pMetricsInstanceCount::count(), expected.size());
  auto &list{Foo::Libp2pMetricsInstanceList::State::get().list};
  EXPECT_EQ(list.size(), expected.size());
  std::set<const Foo *> actual;
  for (auto &foo : list) {
    actual.emplace(foo);
  }
  EXPECT_EQ(actual, expected);
}

/**
 * @given empty metrics
 * @when get Foo metrics
 * @then no Foo
 */
TEST(MetricsInstance, Empty) {
  expectFoo({});
}

/**
 * @given empty metrics
 * @when create one Foo
 * @then one Foo added to metrics
 */
TEST(MetricsInstance, AddOne) {
  Foo foo;
  expectFoo({&foo});
}

/**
 * @given one Foo
 * @when destroy Foo
 * @then Foo removed from metrics
 */
TEST(MetricsInstance, RemoveOne) {
  { Foo foo; }
  expectFoo({});
}

/**
 * @given one Foo
 * @when create second Foo
 * @then second Foo added to metrics
 */
TEST(MetricsInstance, AddSecond) {
  Foo foo1;
  Foo foo2;
  expectFoo({&foo1, &foo2});
}
