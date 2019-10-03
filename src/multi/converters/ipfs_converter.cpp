/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/ipfs_converter.hpp>

#include <string>

#include <libp2p/common/hexutil.hpp>
#include <libp2p/common/types.hpp>
#include <libp2p/multi/converters/conversion_error.hpp>
#include <libp2p/multi/multibase_codec/multibase_codec_impl.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/outcome/outcome.hpp>

using std::string_literals::operator""s;

namespace libp2p::multi::converters {

  auto IpfsConverter::addressToHex(std::string_view addr)
      -> outcome::result<std::string> {
    MultibaseCodecImpl codec;
    std::string encodingStr = ""s
        + static_cast<char>(MultibaseCodecImpl::Encoding::BASE58)
        + std::string(addr);

    OUTCOME_TRY(buf, codec.decode(encodingStr));

    // throw everything in a hex string so we can debug the results
    std::string addr_encoded;

    size_t ilen = 0;
    for (uint8_t const &elem : buf) {
      // get the char so we can see it in the debugger
      auto miu = common::int_to_hex(elem);
      addr_encoded += miu;
    }
    ilen = addr_encoded.length() / 2;
    auto result = UVarint{ilen}.toHex() + addr_encoded;
    return result;
  }

}  // namespace libp2p::multi::converters
