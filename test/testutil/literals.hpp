/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TEST_TESTUTIL_LITERALS_HPP_
#define LIBP2P_TEST_TESTUTIL_LITERALS_HPP_

#include "p2p/common/types.hpp"
#include "p2p/common/hexutil.hpp"
#include "p2p/multi/multiaddress.hpp"
#include "p2p/multi/multihash.hpp"
#include "p2p/peer/peer_id.hpp"

inline libp2p::common::Hash256 operator"" _hash256(const char *c, size_t s) {
  libp2p::common::Hash256 hash{};
  std::copy_n(c, std::min(s, 32ul), hash.rbegin());
  return hash;
}

inline std::vector<uint8_t> operator"" _v(const char *c, size_t s) {
  std::vector<uint8_t> chars(c, c + s);
  return chars;
}

inline std::vector<uint8_t> operator""_unhex(const char *c, size_t s) {
  return libp2p::common::unhex(std::string_view(c, s)).value();
}

inline libp2p::multi::Multiaddress operator""_multiaddr(const char *c,
                                                        size_t s) {
  return libp2p::multi::Multiaddress::create(std::string_view(c, s)).value();
}

/// creates a multihash instance from a hex string
inline libp2p::multi::Multihash operator""_multihash(const char *c, size_t s) {
  return libp2p::multi::Multihash::createFromHex(std::string_view(c, s))
      .value();
}

inline libp2p::peer::PeerId operator""_peerid(const char *c, size_t s) {
  libp2p::crypto::PublicKey p;
  p.data = std::vector<uint8_t>(c, c + s);
  return libp2p::peer::PeerId::fromPublicKey(p);
}

#endif  // LIBP2P_TEST_TESTUTIL_LITERALS_HPP_
