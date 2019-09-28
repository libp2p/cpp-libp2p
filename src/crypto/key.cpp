/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/key.hpp>

#include <boost/container_hash/hash.hpp>

size_t std::hash<libp2p::crypto::Key>::operator()(
    const libp2p::crypto::Key &x) const {
  size_t seed = 0;
  boost::hash_combine(seed, x.type);
  boost::hash_combine(seed, boost::hash_range(x.data.begin(), x.data.end()));
  return seed;
}

size_t std::hash<libp2p::crypto::PrivateKey>::operator()(
    const libp2p::crypto::PrivateKey &x) const {
  return std::hash<libp2p::crypto::Key>()(x);
}

size_t std::hash<libp2p::crypto::PublicKey>::operator()(
    const libp2p::crypto::PublicKey &x) const {
  return std::hash<libp2p::crypto::Key>()(x);
}

size_t std::hash<libp2p::crypto::KeyPair>::operator()(
    const libp2p::crypto::KeyPair &x) const {
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  size_t seed = 0;
  boost::hash_combine(seed, std::hash<PublicKey>()(x.publicKey));
  boost::hash_combine(seed, std::hash<PrivateKey>()(x.privateKey));
  return seed;
}
