/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multiaddress_protocol_list.hpp"

#include <gtest/gtest.h>

using libp2p::multi::Protocol;
using libp2p::multi::ProtocolList;

/**
 * @given a protocol list and the desired protocol name
 * @when acquiring the data about the protocol by its name
 * @then if a protocol with such name exists, a pointer to the data about it is
 * returned
 */
TEST(ProtocolList, getByName) {
  static_assert(ProtocolList::get("ip4")->name == "ip4");
  static_assert(!ProtocolList::get("ip5"));
}

/**
 * @given a protocol list and the desired protocol code
 * @when acquiring the data about the protocol by its code
 * @then if a protocol with such code exists, a pointer to the data about it is
 * returned
 */
TEST(ProtocolList, getByCode) {
  static_assert(ProtocolList::get(Protocol::Code::IP6)->name == "ip6");
  static_assert(ProtocolList::get(Protocol::Code::DCCP)->code
                == Protocol::Code::DCCP);
  static_assert(!ProtocolList::get(static_cast<Protocol::Code>(1312312)));
}

/**
 * @given a ProtocolList
 * @when acquiring the collection of known protocols
 * @then the collection contatining the data about all known protocols is
 * returned
 */
TEST(ProtocolList, getProtocols) {
  auto &protocols = ProtocolList::getProtocols();
  static_assert(protocols.size() == ProtocolList::kProtocolsNum);
  auto it = std::find_if(protocols.begin(), protocols.end(),
                         [](auto &p) { return p.name == "ip4"; });
  ASSERT_NE(it, protocols.end());
}
