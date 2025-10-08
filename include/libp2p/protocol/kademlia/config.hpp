/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/stream_protocols.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  using namespace std::chrono_literals;

  // https://github.com/libp2p/rust-libp2p/blob/e63975d7742710d4498b941e151c5177e06392ce/protocols/kad/src/lib.rs#L93
  constexpr size_t K_VALUE = 20;

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

    struct PeriodicReplication {
      /**
       * True if periodic replication is enabled
       */
      bool enabled = true;

      /**
       * Interval for periodic replication
       * @note Default: 1h
       */
      std::chrono::seconds interval = 1h;

      /**
       * Number of peers to replicate to per cycle
       * @note Default: 3 (subset of K_VALUE)
       */
      size_t peers_per_cycle = 3;
    };

    struct PeriodicRepublishing {
      /**
       * True if periodic republishing is enabled
       */
      bool enabled = true;

      /**
       * Interval for periodic republishing
       * @note Default: 24h
       */
      std::chrono::seconds interval = 24h;

      /**
       * Number of peers to republish to per cycle
       * @note Default: 6 (subset of K_VALUE)
       */
      size_t peers_per_cycle = 6;
    };
  }  // namespace

  class Config {
   public:
    Config() = default;

    /**
     * @returns kademlia protocol ids
     * @note Default: "ipfs/kad/1.0"
     */
    StreamProtocols protocols = {"/ipfs/kad/1.0.0"};

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
    size_t maxBucketSize = K_VALUE;

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

    /**
     * Periodic replication config
     */
    PeriodicReplication periodicReplication{};

    /**
     * Periodic republishing config
     */
    PeriodicRepublishing periodicRepublishing{};

    // https://github.com/libp2p/rust-libp2p/blob/c6cf7fec6913aa590622aeea16709fce6e9c99a5/protocols/kad/src/query/peers/closest.rs#L110-L120
    size_t query_initial_peers = K_VALUE;

    // https://github.com/libp2p/rust-libp2p/blob/9a45db3f82b760c93099e66ec77a7a772d1f6cd3/protocols/kad/src/query/peers/closest.rs#L336-L346
    size_t replication_factor = K_VALUE;
  };

}  // namespace libp2p::protocol::kademlia
