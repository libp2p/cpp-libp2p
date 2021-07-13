/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MULTIHASH_HPP
#define LIBP2P_MULTIHASH_HPP

#include <cstdint>
#include <string>
#include <utility>

#include <gsl/span>
#include <libp2p/common/types.hpp>
#include <libp2p/multi/hash_type.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::multi {

  /**
   * Special format of hash used in Libp2p. Allows to differentiate between
   * outputs of different hash functions. More
   * https://github.com/multiformats/multihash
   */
  class Multihash {
   public:
    Multihash(const Multihash &other) = default;
    Multihash &operator=(const Multihash &other) = default;
    Multihash(Multihash &&other) noexcept = default;
    Multihash &operator=(Multihash &&other) noexcept = default;
    ~Multihash() = default;

    using Buffer = common::ByteArray;

    static constexpr uint8_t kMaxHashLength = 127;

    enum class Error {
      ZERO_INPUT_LENGTH = 1,
      INPUT_TOO_LONG,
      INPUT_TOO_SHORT,
      INCONSISTENT_LENGTH
    };

    /**
     * @brief Creates a multihash from hash type and the hash itself. Note that
     * the max hash length is 127
     * @param type - numerical code of the hash type.
     * @param hash - binary buffer with the hash
     * @return result with the multihash in case of success
     */
    static outcome::result<Multihash> create(HashType type,
                                             gsl::span<const uint8_t> hash);

    /**
     * @brief Creates a multihash from a string, which represents a binary
     * buffer in hexadecimal form. The first byte denotes the hash type, the
     * second one contains the hash length, and the following are the hash
     * itself
     * @param hex - the string with hexadecimal representation of the multihash
     * @return result with the multihash in case of success
     */
    static outcome::result<Multihash> createFromHex(std::string_view hex);

    /**
     * @brief Creates a multihash from a binary
     * buffer. The first byte denotes the hash type, the
     * second one contains the hash length, and the following are the hash
     * itself
     * @param b - the buffer with the multihash
     * @return result with the multihash in case of success
     */
    static outcome::result<Multihash> createFromBytes(
        gsl::span<const uint8_t> b);

    /**
     * @return the info about hash type
     */
    const HashType &getType() const;

    /**
     * @return the hash stored in this multihash
     */
    gsl::span<const uint8_t> getHash() const;

    /**
     * @return a string with hexadecimal representation of the multihash
     */
    std::string toHex() const;

    /**
     * @return a buffer with the multihash, including its type, length and hash
     */
    const Buffer &toBuffer() const;

    /**
     * @return Pre-calculated hash for std containers
     */
    size_t stdHash() const;

    bool operator==(const Multihash &other) const;
    bool operator!=(const Multihash &other) const;
    bool operator<(const Multihash &other) const;

   private:
    /**
     * Header consists of hash type and hash size, both 1 byte or 2 hex digits
     * long, thus 4 hex digits long in total
     */
    static constexpr uint8_t kHeaderSize = 4;

    /**
     * Constructs a multihash from a type and a hash. Inits length_ with the
     * size of the hash. Does no checks on the validity of provided data, in
     * contrary to public fabric methods
     * @param type - the info about the hash type
     * @param hash - a binary buffer with the hash
     */
    Multihash(HashType type, gsl::span<const uint8_t> hash);

    /**
     * Contains a one byte hash type, a one byte hash length, and the stored
     * hash itself
     */
    struct Data {
      // TODO(artem): move to small_vector<const uint8_t, some_size>
      // as soon as toBuffer() -> span<const uint8_t> is acceptable
      std::vector<uint8_t> bytes;
      uint8_t hash_offset{};  ///< size of non-hash data from the beginning
      HashType type;
      size_t std_hash; ///< Hash for unordered containers

      Data(HashType t, gsl::span<const uint8_t> h);
    };

    const Data& data() const;

    std::shared_ptr<const Data> data_;
  };

}  // namespace libp2p::multi

namespace std {
  template <>
  struct hash<libp2p::multi::Multihash> {
    size_t operator()(const libp2p::multi::Multihash &x) const {
      return x.stdHash();
    }
  };
}  // namespace std

OUTCOME_HPP_DECLARE_ERROR(libp2p::multi, Multihash::Error);

#endif  // LIBP2P_MULTIHASH_HPP
