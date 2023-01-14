/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/ipfs_converter.hpp>

#include <string>

#include <boost/algorithm/hex.hpp>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/multi/converters/conversion_error.hpp>
#include <libp2p/multi/multibase_codec/multibase_codec_impl.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/outcome/outcome.hpp>

using std::string_literals::operator""s;

namespace libp2p::multi::converters {

  outcome::result<std::string> IpfsConverter::addressToHex(
      std::string_view addr) {
    std::string encoded =
        static_cast<char>(MultibaseCodecImpl::Encoding::BASE58)
        + std::string(addr);

    MultibaseCodecImpl codec;
    auto decoded_res = codec.decode(encoded);
    if (decoded_res.has_error()) {
      return ConversionError::INVALID_ADDRESS;
    }
    auto &decoded = decoded_res.value();

    // throw everything in a hex string so we can debug the results
    std::string hex = UVarint(decoded.size()).toHex();
    hex.reserve(hex.size() + addr.size() * 2);
    boost::algorithm::hex_lower(decoded.begin(), decoded.end(),
                                std::back_inserter(hex));
    return std::move(hex);
  }

  outcome::result<common::ByteArray> IpfsConverter::addressToBytes(
      std::string_view addr) {
    std::string encoded =
        static_cast<char>(MultibaseCodecImpl::Encoding::BASE58)
        + std::string(addr);

    MultibaseCodecImpl codec;
    auto decoded_res = codec.decode(encoded);
    if (decoded_res.has_error()) {
      return ConversionError::INVALID_ADDRESS;
    }
    auto &decoded = decoded_res.value();

    auto bytes = UVarint(decoded.size()).toVector();
    bytes.reserve(bytes.size() + decoded.size());
    bytes.insert(bytes.end(), decoded.begin(), decoded.end());
    return std::move(bytes);
  }

}  // namespace libp2p::multi::converters
