/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/ipfs_converter.hpp>

#include <string>

#include <libp2p/common/types.hpp>
#include <libp2p/multi/converters/conversion_error.hpp>
#include <libp2p/multi/multibase_codec/multibase_codec_impl.hpp>
#include <libp2p/multi/uvarint.hpp>

using std::string_literals::operator""s;

namespace libp2p::multi::converters {
  outcome::result<Bytes> IpfsConverter::addressToBytes(std::string_view addr) {
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
