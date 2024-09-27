/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/peer/errors.hpp>
#include <libp2p/peer/protocol_repository.hpp>
#include <libp2p/peer/protocol_repository/inmem_protocol_repository.hpp>
#include <qtils/test/outcome.hpp>

using namespace libp2p::peer;
using namespace libp2p::common;
using libp2p::multi::HashType;
using libp2p::multi::Multihash;

struct InmemProtocolRepository_Test : public ::testing::Test {
  std::unique_ptr<ProtocolRepository> db;

  void SetUp() override {
    db = std::make_unique<InmemProtocolRepository>();
  }

  const ProtocolName s1 = "/bittorrent.org/1.0";
  const ProtocolName s2 = "/ipfs/1.0";

  const PeerId p1 = PeerId::fromHash("12051203020304"_multihash).value();
  const PeerId p2 = PeerId::fromHash("12051203FFFFFF"_multihash).value();

  template <typename... T>
  std::vector<ProtocolName> vec(T &&...arg) {
    return std::vector<ProtocolName>{arg...};
  }

  template <typename... T>
  std::set<ProtocolName> set(T &&...arg) {
    return std::set<ProtocolName>{arg...};
  }
};

/**
 * @given empty repository
 * @when add two protocols
 * @then two protocols added
 */
TEST_F(InmemProtocolRepository_Test, Add) {
  EXPECT_OK(db->addProtocols(p1, vec(s1, s2)));
  {
    auto v = EXPECT_OK(db->getProtocols(p1));
    EXPECT_EQ(v.size(), 2);

    auto r = db->getProtocols(p2);
    EXPECT_EC(r, PeerError::NOT_FOUND);
  }
}

/**
 * @given two protocols are added to p1, p2 has no protocols
 * @when collect garbage on empty peers
 * @then they are evicted
 */
TEST_F(InmemProtocolRepository_Test, CollectGarbage) {
  EXPECT_OK(db->addProtocols(p1, vec(s1, s2)));
  EXPECT_OK(db->addProtocols(p2, vec()));

  // no effect
  db->collectGarbage();

  {
    auto v = EXPECT_OK(db->getProtocols(p1));
    EXPECT_EQ(v.size(), 2);

    auto r = db->getProtocols(p2);
    EXPECT_EC(r, PeerError::NOT_FOUND);
  }

  // clear p1. now p1 has 0 protocols
  db->clear(p1);

  // evicts p1 and p2
  db->collectGarbage();

  {
    for (const auto &it : {p1, p2}) {
      auto r = db->getProtocols(it);
      EXPECT_EC(r, PeerError::NOT_FOUND);
    }
  }
}

/**
 * @given database with s1 and s2
 * @when supportsProtocols executed with different patterns
 * @then expected protocols are returned
 */
TEST_F(InmemProtocolRepository_Test, Supports) {
  EXPECT_OK(db->addProtocols(p1, vec(s1, s2)));

  // one of
  {
    auto v = EXPECT_OK(db->supportsProtocols(p1, set(s1)));
    EXPECT_EQ(v, vec(s1));
  }

  // forward order
  {
    auto v = EXPECT_OK(db->supportsProtocols(p1, set(s1, s2)));
    EXPECT_EQ(v, vec(s1, s2));
  }

  // reverse order
  {
    auto v = EXPECT_OK(db->supportsProtocols(p1, set(s2, s1)));
    EXPECT_EQ(v, vec(s1, s2));
  }

  // non existing
  {
    EXPECT_OK(db->removeProtocols(p1, vec(s1)));
    auto v = EXPECT_OK(db->supportsProtocols(p1, set(s1, s2)));
    EXPECT_EQ(v, vec(s2));
  }
}

/**
 * @given p1 with s1 and s2
 * @when remove s1 and get all protocols
 * @then protocol s1 is removed
 */
TEST_F(InmemProtocolRepository_Test, Remove) {
  EXPECT_OK(db->addProtocols(p1, vec(s1, s2)));
  EXPECT_OK(db->removeProtocols(p1, vec(s1)));
  auto v = EXPECT_OK(db->getProtocols(p1));
  EXPECT_EQ(v, vec(s2));
}

/**
 * @given peer p1 with s2
 * @when remove non-existing protocol s1
 * @then get no error
 */
TEST_F(InmemProtocolRepository_Test, RemoveNonExisting) {
  EXPECT_OK(db->addProtocols(p1, vec(s2)));
  EXPECT_OK(db->removeProtocols(p1, vec(s1)));
  auto v = EXPECT_OK(db->getProtocols(p1));
  EXPECT_EQ(v.size(), 1);
}

/**
 * @given 2 peers in storage
 * @when get peers
 * @then 2 peers returned
 */
TEST_F(InmemProtocolRepository_Test, GetPeers) {
  EXPECT_OK(db->addProtocols(p1, {}));
  auto s1 = db->getPeers();
  EXPECT_EQ(s1.size(), 1);

  EXPECT_OK(db->addProtocols(p2, {}));
  auto s = db->getPeers();
  EXPECT_EQ(s.size(), 2);
}
