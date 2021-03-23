/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/node_id.hpp>
#include <libp2p/protocol/kademlia/impl/peer_routing_table_impl.hpp>

#include <gtest/gtest.h>
#include <cstdlib>
#include <iostream>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/common/literals.hpp>
#include "testutil/libp2p/peer.hpp"

using namespace libp2p::common;

using libp2p::peer::PeerId;
using libp2p::protocol::kademlia::NodeId;
using libp2p::protocol::kademlia::xor_distance;
using libp2p::protocol::kademlia::XorDistanceComparator;
using libp2p::protocol::kademlia::BucketPeerInfo;

// allows to print debug output to stdout, not wanted in CI output, but useful
// while debugging
bool verbose() {
  static const bool v = (std::getenv("TRACE_DEBUG") != nullptr);
  return v;
}

bool is_distance_less(Hash256 a, Hash256 b) {
  constexpr auto size = Hash256().size();
  return std::memcmp(a.data(), b.data(), size) < 0;
}

bool is_xor_distance_sorted(const PeerId &local, std::vector<BucketPeerInfo> &peers) {
  NodeId nlocal(local);

  auto begin = peers.begin();
  auto end = peers.end();
  while (begin != end) {
    const auto& nremote1 = (*begin).node_id;
    auto distance1 = nremote1.distance(nlocal);

    if (++begin == end) {
      continue;
    }

    const auto& nremote2 = (*begin).node_id;
    auto distance2 = nremote2.distance(nlocal);

    if (!is_distance_less(distance1, distance2)) {
      return false;
    }
  }

  return true;
}

void print(NodeId from, std::vector<BucketPeerInfo> &pids) {
  if (!verbose())
    return;
  std::cout << "peers: \n";
  for (auto &p : pids) {
    std::cout << "pid: " << p.peer_id.toHex()
              << " nodeId: " << hex_upper(p.node_id.getData())
              << " distance: " << hex_upper(from.distance(p.node_id)) << "\n";
  }
}

TEST(KadDistance, SortsHashes) {
  size_t peersTotal = 1000;
  srand(0);  // make test deterministic
  PeerId us = "1"_peerid;
  XorDistanceComparator comp(us);

  std::vector<BucketPeerInfo> peers;
  std::generate_n(std::back_inserter(peers), peersTotal,
                  []() { return BucketPeerInfo(testutil::randomPeerId(),false); });
  peers.emplace_back(us, false);

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
