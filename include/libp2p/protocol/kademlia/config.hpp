/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_CONFIG
#define LIBP2P_PROTOCOL_KADEMLIA_CONFIG

#include <chrono>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::protocol::kademlia {

  using namespace std::chrono_literals;

  class Config {
   public:
    virtual ~Config() = default;

    /**
     * @returns delay between kicks off bootstrap
     * @note Default: 5 minutes
     */
    virtual std::chrono::seconds bootstrapKickOfInterval() const = 0;

    /**
     * @returns how many times of 'Node lookups' process should be done per
     * bootstrap
     * @note Default: 1 time
     */
    virtual size_t queryCount() const = 0;

    /**
     * @returns how many times of 'Node lookups' process should be done per
     * bootstrap
     * @note Default: 10 seconds
     */
    virtual size_t queryTimeout() const = 0;

    /**
     * @returns minimal responses from distinct nodes to check for consistency
     * before returning an answer.
     * @note Default: 0
     */
    virtual size_t valueLookupsQuorum() const = 0;

    /**
     * @returns TTL of record in storage
     * @note Default: 24h
     */
    virtual std::chrono::seconds storageRecordTTL() const = 0;

    /**
     * @returns Interval of wiping expired records
     * @note Default: 1h
     */
    virtual std::chrono::seconds storageWipingInterval() const = 0;

    /**
     * @returns Interval of refresh storage.
     * This is implementation specified property.
     * @note Default: 5m
     */
    virtual std::chrono::seconds storageRefreshInterval() const = 0;

    /**
     * @returns Max size of bucket
     * This is implementation specified property.
     * @note Default: 20
     */
    virtual size_t maxBucketSize() const = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_ROUTING
