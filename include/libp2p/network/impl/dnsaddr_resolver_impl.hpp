/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_NETWORK_IMPL_DNSADDR_RESOLVER_IMPL_HPP
#define LIBP2P_INCLUDE_LIBP2P_NETWORK_IMPL_DNSADDR_RESOLVER_IMPL_HPP

#include <libp2p/network/dnsaddr_resolver.hpp>

#include <memory>
#include <string>

#include <boost/asio.hpp>
#include <libp2p/network/cares/cares.hpp>

namespace libp2p::network {

  class DnsaddrResolverImpl : public DnsaddrResolver {
   public:
    static constexpr auto kDnsaddr = multi::Protocol::Code::DNS_ADDR;

    enum class Error {
      INVALID_DNSADDR = 1,
      MALFORMED_RESPONSE,
      BAD_ADDR_IN_RESPONSE,
    };

    DnsaddrResolverImpl(std::shared_ptr<boost::asio::io_context> io_context,
                        const c_ares::Ares &cares);

    void load(multi::Multiaddress address, AddressesCallback callback) override;

   private:
    /// Convert multiaddr "/dnsaddr/hostname" to string "_dnsaddr.hostname"
    static outcome::result<std::string> dnsaddrUriFromMultiaddr(
        const multi::Multiaddress &address);

    std::shared_ptr<boost::asio::io_context> io_context_;
    // captured by reference intentionally to force DI use the single instance
    const c_ares::Ares &cares_;
  };

}  // namespace libp2p::network

OUTCOME_HPP_DECLARE_ERROR(libp2p::network, DnsaddrResolverImpl::Error);

#endif  // LIBP2P_INCLUDE_LIBP2P_NETWORK_IMPL_DNSADDR_RESOLVER_IMPL_HPP
