/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_CONFIG
#define LIBP2P_PROTOCOL_KADEMLIA_CONFIG

#include <chrono>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  using namespace std::chrono_literals;

  namespace {
    struct RandomWalk {
      bool enabled = true;
      size_t queries_per_period = 1;
      std::chrono::seconds interval = 30s;
      std::chrono::seconds timeout = 10s;
      std::chrono::seconds delay = 10s;
    };
  }  // namespace

  class Config {
   public:
    Config() = default;

    /**
     * @returns kademlia protocol id
     * @note Default: "ipfs/kad/1.0"
     */
    std::string_view protocolId = "/ipfs/kad/1.0.0";

    /**
     * @returns delay between kicks off bootstrap
     * @note Default: 5 minutes
     */
    std::chrono::seconds bootstrapKickOfInterval = 5min;

    /**
     * @returns how many times of 'Node lookups' process should be done per
     * bootstrap
     * @note Default: 1 time
     */
    size_t queryCount = 1;

    /**
     * @returns how many times of 'Node lookups' process should be done per
     * bootstrap
     * @note Default: 10
     */
    size_t queryTimeout = 10;

    /**
     * @returns minimal responses from distinct nodes to check for consistency
     * before returning an answer.
     * @note Default: 0
     */
    size_t valueLookupsQuorum = 0;


	  /**
		 * @returns man number of concurrent request
		 * @note Default: 3
		 */
	  size_t requestConcurency = 3;


	  /**
		 * @returns count of closer peers.
		 * @note Default: 6
		 */
	  size_t closerPeerCount = 6;


    /**
     * @returns TTL of record in storage
     * @note Default: 24h
     */
    std::chrono::seconds storageRecordTTL = 24h;

    /**
     * @returns Interval of wiping expired records
     * @note Default: 1h
     */
    std::chrono::seconds storageWipingInterval = 1h;

    /**
     * @returns Interval of refresh storage.
     * This is implementation specified property.
     * @note Default: 5m
     */
    std::chrono::seconds storageRefreshInterval = 5min;

    /**
     * @returns Max size of bucket
     * This is implementation specified property.
     * @note Default: 20
     */
    size_t maxBucketSize = 20;

    /**
     * @returns Random walk config
     */
    RandomWalk randomWalk{};
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_ROUTING
