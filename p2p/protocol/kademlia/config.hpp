/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KADEMLIA_CONFIG_HPP
#define KAGOME_KADEMLIA_CONFIG_HPP

#include <chrono>
#include <cstddef>
#include <string_view>

/**
 * Default constants and settings specified here.
 *
 * https://sourcegraph.com/github.com/libp2p/js-libp2p-kad-dht/-/blob/src/constants.js#L31:51
 */

namespace libp2p::protocol::kademlia {

  using std::chrono_literals::operator""h;    // hours
  using std::chrono_literals::operator""min;  // minutes
  using std::chrono_literals::operator""s;    // seconds

  constexpr std::string_view protocolId = "/ipfs/kad/1.0.0";

  struct RandomWalk {
    bool enabled = true;
    size_t queries_per_period = 1;
    std::chrono::seconds interval = 5min;
    std::chrono::seconds timeout = 10s;
    std::chrono::seconds delay = 10s;
  };

  struct KademliaConfig {
    RandomWalk randomWalk{};

    std::chrono::seconds max_record_age = 36h;

    std::string_view providers_prefix = "/providers/";

    size_t providers_lru_cache_size = 256;

    std::chrono::seconds providers_validity = 24h;

    std::chrono::seconds providers_cleanup_interval = 1h;

    std::chrono::seconds read_message_timeout = 10s;

    /// The number of records that will be retrieved on a call to getMany()
    size_t get_many_records_count = 16;

    /// K is the maximum number of requests to perform before returning failure
    size_t K = 20;

    /// Alpha is the concurrency for asynchronous requests
    size_t ALPHA = 3;

    size_t max_message_size = 2u << 22u;  // 4 MB
  };

}  // namespace libp2p::protocol::kademlia

#endif  // KAGOME_KADEMLIA_CONFIG_HPP
