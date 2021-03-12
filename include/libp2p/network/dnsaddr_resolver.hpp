/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_NETWORK_DNSADDR_RESOLVER_HPP
#define LIBP2P_INCLUDE_LIBP2P_NETWORK_DNSADDR_RESOLVER_HPP

#include <functional>
#include <vector>

#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/peer/peer_info.hpp>

namespace libp2p::network {

  /**
   * Utility for peers discovering through bootstrap addresses like
   * /dnsaddr/<hostname>
   */
  class DnsaddrResolver {
   public:
    virtual ~DnsaddrResolver() = default;

    /**
     * Callback to dnsaddr resolution request.
     * @param a set of discovered addresses
     */
    using AddressesCallback =
        std::function<void(outcome::result<std::vector<multi::Multiaddress>>)>;

    /**
     * Makes a query for peers info to a specified bootstrap address via
     * /dnsaddr
     * @param address - bootstrap address to query for peers
     * @param cb - a callback to fed the discovered peers to
     */
    virtual void load(multi::Multiaddress address, AddressesCallback callback) = 0;
  };
}  // namespace libp2p::network

#endif  // LIBP2P_INCLUDE_LIBP2P_NETWORK_DNSADDR_RESOLVER_HPP
