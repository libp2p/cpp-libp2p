/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "libp2p/protocol/kademlia/node_id.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include "testutil/libp2p/peer.hpp"
#include "testutil/literals.hpp"

using kagome::common::Hash256;
using libp2p::peer::PeerId;
using libp2p::protocol::kademlia::NodeId;
using libp2p::protocol::kademlia::xor_distance;
using libp2p::protocol::kademlia::XorDistanceComparator;

bool is_distance_less(Hash256 a, Hash256 b) {
  return std::memcmp(a.data(), b.data(), Hash256::size()) < 0;
}

bool is_xor_distance_sorted(const PeerId &local, std::vector<PeerId> &peers) {
  NodeId nlocal(local);

  auto begin = peers.begin();
  auto end = peers.end();
  while (begin != end) {
    NodeId nremote1(*begin);
    auto distance1 = nremote1.distance(nlocal);

    if (++begin == end) {
      continue;
    }

    NodeId nremote2(*begin);
    auto distance2 = nremote2.distance(nlocal);

    if (!is_distance_less(distance1, distance2)) {
      return false;
    }
  }

  return true;
}

void print(NodeId from, std::vector<PeerId> &pids) {
  std::cout << "peers: \n";
  for (auto &p : pids) {
    std::cout << "pid: " << p.toHex()
              << " nodeId: " << NodeId(p).getData().toHex()
              << " distance: " << from.distance(NodeId(p)).toHex() << "\n";
  }
}

TEST(KadDistance, SortsHashes) {
  size_t peersTotal = 1000;
  srand(0);  // make test deterministic
  PeerId us = "1"_peerid;
  XorDistanceComparator comp(us);

  std::vector<PeerId> peers;
  std::generate_n(std::back_inserter(peers), peersTotal, []() {
    return testutil::randomPeerId();
  });
  peers.push_back(us);

  ASSERT_EQ(peers.size(), peersTotal + 1);
  std::cout << "unsorted ";
  print(NodeId(us), peers);

  ASSERT_FALSE(is_xor_distance_sorted(us, peers));

  std::cout << "sorting...\n";
  std::sort(peers.begin(), peers.end(), comp);

  std::cout << "sorted ";
  print(NodeId(us), peers);
  ASSERT_TRUE(is_xor_distance_sorted(us, peers));
}
