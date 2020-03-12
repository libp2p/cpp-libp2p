/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MULTIBASE_HPP
#define LIBP2P_MULTIBASE_HPP

#include <string>
#include <string_view>
#include <utility>

#include <libp2p/common/types.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::multi {
  /**
   * Allows to distinguish between different base-encoded binaries
   * See more: https://github.com/multiformats/multibase
   */
  class MultibaseCodec {
   public:
    using ByteBuffer = common::ByteArray;
    using FactoryResult = outcome::result<ByteBuffer>;

    virtual ~MultibaseCodec() = default;
    /**
     * Encodings, supported by this Multibase
     */
    enum class Encoding : char {
      BASE16_LOWER = 'f',
      BASE16_UPPER = 'F',
      BASE32_LOWER = 'b',
      BASE32_UPPER = 'B',
      BASE58 = 'z',
      BASE64 = 'm'
    };

    /**
     * Encode the incoming bytes
     * @param bytes to be encoded
     * @param encoding - base of the desired encoding
     * @return encoded string WITH an encoding prefix
     */
    virtual std::string encode(const ByteBuffer &bytes,
                               Encoding encoding) const = 0;

    /**
     * Decode the incoming string
     * @param string to be decoded
     * @return bytes, if decoding was successful, error otherwise
     */
    virtual FactoryResult decode(std::string_view string) const = 0;
  };

}  // namespace libp2p::multi

#endif  // LIBP2P_MULTIBASE_HPP
