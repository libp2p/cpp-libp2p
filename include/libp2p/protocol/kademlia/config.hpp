/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_CONFIG
#define LIBP2P_PROTOCOL_KADEMLIA_CONFIG

#include <libp2p/outcome/outcome.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  using namespace std::chrono_literals;

  namespace {
    struct RandomWalk {
      /**
       * True if random walking is enabled
       */
      bool enabled = true;

      /**
       * Number of random walks for a period
       * @note Default: 1
       */
      size_t queries_per_period = 1;

      /**
       * Period of random walks' series
       * @note Default: 5min
       */
      std::chrono::seconds interval = 30s;

      /**
       * Timeout for one random walk
       * @note Default: 10s
       */
      std::chrono::seconds timeout = 10s;

      /**
       * Delay beetween random walks in serie
       * @note Default: 10s
       */
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
    peer::ProtocolName protocolId = "/ipfs/kad/1.0.0";

    /**
     * True if application is not announce himself
     */
    bool passiveMode = false;

    /**
     * Minimal responses from distinct nodes to check for consistency before
     * returning an answer.
     * @note Default: 0
     */
    size_t valueLookupsQuorum = 0;

    /**
     * Maximum number of concurrent request
     * @note Default: 3
     */
    size_t requestConcurency = 3;

    /**
     * Target amount of closer peers.
     * @note Default: 6
     */
    size_t closerPeerCount = 6;

    /**
     * TTL of record in storage
     * @note Default: 24h
     */
    std::chrono::seconds storageRecordTTL = 24h;

    /**
     * Interval of wiping expired records
     * @note Default: 1h
     */
    std::chrono::seconds storageWipingInterval = 1h;

    /**
     * Interval of refresh storage.
     * This is implementation specified property.
     * @note Default: 5m
     */
    std::chrono::seconds storageRefreshInterval = 5min;

    /**
     * TTL of provider record
     * @note Default: 24h
     */
    std::chrono::seconds providerRecordTTL = 24h;

    /**
     * Interval of wiping expired provider records
     * @note Default: 1h
     */
    std::chrono::seconds providerWipingInterval = 1h;

    /**
     * Max providers number per one key
     * @note Default: 6
     */
    size_t maxProvidersPerKey = 6;

    /**
     * Maximum size of bucket
     * This is implementation specified property.
     * @note Default: 20
     */
    size_t maxBucketSize = 20;

    /**
     * Maximum time to waiting response
     * This is implementation specified property.
     * @note Default: 10s
     */
    std::chrono::seconds responseTimeout = 10s;

    /**
     * Maximum time to connection
     * This is implementation specified property.
     * @note Default: 3s
     */
    std::chrono::seconds connectionTimeout = 3s;

    /**
     * Random walk config
     */
    RandomWalk randomWalk{};
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_ROUTING
