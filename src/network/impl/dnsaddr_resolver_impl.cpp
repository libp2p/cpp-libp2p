/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>

#include <libp2p/network/impl/dnsaddr_resolver_impl.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::network, DnsaddrResolverImpl::Error, e) {
  using E = libp2p::network::DnsaddrResolverImpl::Error;
  switch (e) {
    case E::INVALID_DNSADDR:
      return "Supplied multiaddress is not a valid /dnsaddr";
    case E::MALFORMED_RESPONSE:
      return "Response format does not comply with the specification. Each "
             "line should be prefixed with dnsaddr=";
    case E::BAD_ADDR_IN_RESPONSE:
      return "Response contains records which are not multiaddresses";
    default:
      return "unknown error";
  }
}

namespace libp2p::network {
  DnsaddrResolverImpl::DnsaddrResolverImpl(
      std::shared_ptr<boost::asio::io_context> io_context,
      const c_ares::Ares &cares)
      : io_context_{std::move(io_context)}, cares_{cares} {
    BOOST_ASSERT(io_context_);
  }

  void DnsaddrResolverImpl::load(multi::Multiaddress address,
                                 DnsaddrResolver::AddressesCallback callback) {
    auto host_uri_res = dnsaddrUriFromMultiaddr(address);
    if (host_uri_res.has_error()) {
      callback(host_uri_res.error());
      return;
    }
    auto &&host_uri = host_uri_res.value();
    auto handler =
        [cb{std::move(callback)}](
            outcome::result<std::vector<std::string>> response) -> void {
      if (response.has_error()) {
        cb(response.error());
        return;
      }
      auto &&lines = response.value();
      // check all records are dnsaddr= response
      bool prefixed =
          std::all_of(lines.begin(), lines.end(), [](const std::string &line) {
            return 0 == line.rfind("dnsaddr=", 0);
          });
      if (not prefixed) {
        cb(Error::MALFORMED_RESPONSE);
        return;
      }
      std::transform(lines.begin(), lines.end(), lines.begin(),
                     [](const std::string &line) -> std::string {
                       return line.substr(8);  // drop first "dnsaddr=" chars
                     });
      std::vector<multi::Multiaddress> addresses;
      addresses.reserve(lines.size());
      for (const auto &line : lines) {
        auto res = multi::Multiaddress::create(line);
        if (res.has_error()) {
          cb(Error::BAD_ADDR_IN_RESPONSE);
          return;
        }
        addresses.emplace_back(std::move(res.value()));
      }
      cb(std::move(addresses));
    };

    cares_.resolveTxt(host_uri, io_context_, handler);
  }

  outcome::result<std::string> DnsaddrResolverImpl::dnsaddrUriFromMultiaddr(
      const multi::Multiaddress &address) {
    if (not address.hasProtocol(kDnsaddr)) {
      return Error::INVALID_DNSADDR;
    }
    OUTCOME_TRY(hostname, address.getFirstValueForProtocol(kDnsaddr));
    return "_dnsaddr." + hostname;
  }
}  // namespace libp2p::network
