/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PEER_ID_HPP
#define LIBP2P_PEER_ID_HPP

#include <libp2p/crypto/key.hpp>
#include <libp2p/crypto/protobuf/protobuf_key.hpp>
#include <libp2p/multi/multihash.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::peer {

  /**
   * Unique identifier of the peer - SHA256 Multihash, in most cases, of its
   * public key
   */
  class PeerId {
    using FactoryResult = outcome::result<PeerId>;

   public:
    PeerId(const PeerId &other) = default;
    PeerId &operator=(const PeerId &other) = default;
    PeerId(PeerId &&other) noexcept = default;
    PeerId &operator=(PeerId &&other) noexcept = default;
    ~PeerId() = default;

    enum class FactoryError { SUCCESS = 0, SHA256_EXPECTED = 1 };

    /**
     * Create a PeerId from the public key
     * @param key, from which PeerId is to be created
     * @return instance of PeerId
     */
    static FactoryResult fromPublicKey(const crypto::ProtobufKey &key);

    /**
     * Create a PeerId from the byte array (serialized multihash).
     * @param v buffer
     * @return instance of PeerId
     */
    static FactoryResult fromBytes(gsl::span<const uint8_t> v);

    /**
     * Create a PeerId from base58-encoded string (not Multibase58!) with its
     * SHA256-hashed ID
     * @param id, with which PeerId is to be created
     * @return instance of PeerId in case of success, error otherwise
     */
    static FactoryResult fromBase58(std::string_view id);

    /**
     * Create a PeerId from SHA256 hash of its ID
     * @param hash, with which PeerId is to be created; MUST be SHA256
     * @return instance of PeerId in case of success, error otherwise
     */
    static FactoryResult fromHash(const multi::Multihash &hash);

    /**
     * Get a base58 (not Multibase58!) representation of this PeerId
     * @return base58-encoded SHA256 multihash of the peer's ID
     */
    std::string toBase58() const;

    /**
     * @brief Get a hex representation of this PeerId.
     */
    std::string toHex() const;

    /**
     * Creates a vector representation of PeerId.
     */
    const std::vector<uint8_t>& toVector() const;

    /**
     * Get a SHA256 multihash of the peer's ID
     * @return multihash
     */
    const multi::Multihash &toMultihash() const;

    bool operator<(const PeerId &other) const;
    bool operator==(const PeerId &other) const;
    bool operator!=(const PeerId &other) const;

   private:
    /// if key, from which a PeerId is created, does not exceed this size, it's
    /// put as a PeerId as-is, without SHA-256 hashing
    static constexpr size_t kMaxInlineKeyLength = 42;

    /**
     * Create an instance of PeerId
     * @param hash, with which PeerId is to be created
     */
    explicit PeerId(multi::Multihash hash);

    multi::Multihash hash_;
  };

}  // namespace libp2p::peer

namespace std {
  template <>
  struct hash<libp2p::peer::PeerId> {
    size_t operator()(const libp2p::peer::PeerId &peer_id) const;
  };
}  // namespace std

OUTCOME_HPP_DECLARE_ERROR(libp2p::peer, PeerId::FactoryError)

#endif  // LIBP2P_PEER_ID_HPP
