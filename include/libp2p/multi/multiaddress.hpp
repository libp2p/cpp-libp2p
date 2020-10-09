/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MULTIADDRESS_HPP
#define LIBP2P_MULTIADDRESS_HPP

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <gsl/span>
#include <boost/optional.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/multi/multiaddress_protocol_list.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::multi {

  /**
   * Address format, used by Libp2p
   */
  class Multiaddress {
   private:
    using ByteBuffer = common::ByteArray;
    using FactoryResult = outcome::result<Multiaddress>;

   public:
    Multiaddress() = delete;

    enum class Error {
      INVALID_INPUT = 1,      ///< input contains invalid multiaddress
      PROTOCOL_NOT_FOUND,     ///< given protocol can not be found
      INVALID_PROTOCOL_VALUE  ///< protocol value can not be casted to T
    };

    /**
     * Construct a multiaddress instance from the string
     * @param address - string to be in that multiaddress
     * @return pointer to Multiaddress, if creation is successful, error
     * otherwise
     */
    static FactoryResult create(std::string_view address);

    /**
     * Construct a multiaddress instance from the bytes
     * @param bytes to be in that multiaddress
     * @return pointer to Multiaddress, if creation is successful, error
     * otherwise
     */
    static FactoryResult create(const ByteBuffer &bytes);

    /**
     * Construct a multiaddress instance from the bytes
     * @param bytes to be in that multiaddress
     * @return pointer to Multiaddress, if creation is successful, error
     * otherwise
     */
    static FactoryResult create(gsl::span<const uint8_t> bytes);

    /**
     * Encapsulate a multiaddress to this one, such that:
     * '/ip4/192.168.0.1' after encapsulation with '/udp/138' becomes
     * '/ip4/192.168.0.1/udp/138'
     * @param address - another address to be encapsulated into this one
     */
    void encapsulate(const Multiaddress &address);

    /**
     * Decapsulate a multiaddress from this one, such that:
     * '/ip4/192.168.0.1/udp/138' after decapsulation with '/udp/' becomes
     * '/ip4/192.168.0.1'
     * @param address - another address to be decapsulated from this one
     * @return true, if such address was found and removed, false otherwise
     */
    bool decapsulate(const Multiaddress &address);

    /**
     * @see decapsulate(Multiaddress)
     * @param proto - protocol, the last occurrence of which will be
     * decapsulated
     * @return true, if such protocol was found and removed, false otherwise
     */
    bool decapsulate(Protocol::Code proto);

    /**
     * Split the Multiaddress by the first protocol, such that:
     * '/ip4/192.168.0.1/tcp/228' => <'/ip4/192.168.0.1', '/tcp/228'>
     * @return pair of addresses; if there's only one protocol in the provided
     * address, the second element will be none
     */
    std::pair<Multiaddress, boost::optional<Multiaddress>> splitFirst() const;

    /**
     * @brief Tests if {@param code} exists in this multiaddr.
     * @param code protocol to be tested
     * @return true if exists, false otherwise
     */
    bool hasProtocol(Protocol::Code code) const;

    /**
     * Get the textual representation of the address inside
     * @return stringified address
     */
    std::string_view getStringAddress() const;

    /**
     * Get the byte representation of the address inside
     * @return bytes address
     */
    const ByteBuffer &getBytesAddress() const;

    /**
     * Get peer id of this Multiaddress
     * @return peer id if exists
     */
    boost::optional<std::string> getPeerId() const;

    /**
     * Get all values, which are under that protocol in this multiaddress
     * @param proto to be searched for
     * @return empty vector if no protocols found, or vector with values
     * otherwise
     */
    std::vector<std::string> getValuesForProtocol(Protocol::Code proto) const;

    /**
     * Get first value for protocol
     * @param proto to be searched for
     * @return value (string) if protocol is found, none otherwise
     */
    outcome::result<std::string> getFirstValueForProtocol(
        Protocol::Code proto) const;

    /**
     * Get protocols contained in the multiaddress. Repetitions are possible
     * @return list of contained protocols
     */
    std::list<Protocol> getProtocols() const;

    /**
     * Get protocols contained in the multiaddress and values assosiated with
     * them (usually addresses). Repetitions are possible.
     * @return list of pairs with a protocol as the first element and the value
     * as the second one
     */
    std::vector<std::pair<Protocol, std::string>> getProtocolsWithValues()
        const;

    bool operator==(const Multiaddress &other) const;

    /**
     * Lexicographical comparison of string representations of the
     * Multiaddresses
     */
    bool operator<(const Multiaddress &other) const;

    template <typename T>
    outcome::result<T> getFirstValueForProtocol(
        Protocol::Code protocol,
        std::function<T(const std::string &)> caster) const {
      OUTCOME_TRY(val, getFirstValueForProtocol(protocol));

      try {
        return caster(val);
      } catch (...) {
        return Error::INVALID_PROTOCOL_VALUE;
      }
    }

   private:
    /**
     * Construct a multiaddress instance from both address and bytes
     * @param address to be in the multiaddress
     * @param bytes to be in the multiaddress
     */
    Multiaddress(std::string &&address, ByteBuffer &&bytes);

    /**
     * Decapsulate a given string, which represents a protocol, from the address
     * @return true, if it was found and removed, false otherwise
     */
    bool decapsulateStringFromAddress(std::string_view proto,
                                      const ByteBuffer &bytes);

    std::string stringified_address_;
    ByteBuffer bytes_;

    boost::optional<std::string> peer_id_;
  };
}  // namespace libp2p::multi

namespace std {
  template <>
  struct hash<libp2p::multi::Multiaddress> {
    size_t operator()(const libp2p::multi::Multiaddress &x) const;
  };
}  // namespace std

OUTCOME_HPP_DECLARE_ERROR(libp2p::multi, Multiaddress::Error)

#endif  // LIBP2P_MULTIADDRESS_HPP
