/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/literals.hpp>

#include <libp2p/common/hexutil.hpp>
#include <libp2p/crypto/protobuf/protobuf_key.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/multi/multihash.hpp>
#include <libp2p/peer/peer_id.hpp>

#include <algorithm>

namespace libp2p::common {
  libp2p::common::Hash256 operator""_hash256(const char *c, size_t s) {
    libp2p::common::Hash256 hash{};
    std::copy_n(c, std::min(s, 32ul), hash.rbegin());
    return hash;
  }

  libp2p::common::Hash512 operator""_hash512(const char *c, size_t s) {
    libp2p::common::Hash512 hash{};
    std::copy_n(c, std::min(s, 64ul), hash.rbegin());
    return hash;
  }

  std::vector<uint8_t> operator""_v(const char *c, size_t s) {
    std::vector<uint8_t> chars(c, c + s);  // NOLINT
    return chars;
  }

  std::vector<uint8_t> operator""_unhex(const char *c, size_t s) {
    return libp2p::common::unhex(std::string_view(c, s)).value();
  }

  libp2p::multi::Multiaddress operator""_multiaddr(const char *c, size_t s) {
    return libp2p::multi::Multiaddress::create(std::string_view(c, s)).value();
  }

  libp2p::multi::Multihash operator""_multihash(const char *c, size_t s) {
    return libp2p::multi::Multihash::createFromHex(std::string_view(c, s))
        .value();
  }

  libp2p::peer::PeerId operator""_peerid(const char *c, size_t s) {
    libp2p::crypto::PublicKey p;
    p.data = std::vector<uint8_t>(c, c + s);  // NOLINT
    return libp2p::peer::PeerId::fromPublicKey(
               libp2p::crypto::ProtobufKey{p.data})
        .value();
  }
}  // namespace libp2p::common
