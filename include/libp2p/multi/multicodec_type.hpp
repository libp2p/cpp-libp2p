/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MULTICODECTYPE_HPP
#define LIBP2P_MULTICODECTYPE_HPP

#include <string>

#include <boost/optional.hpp>

namespace libp2p::multi {

  /**
   * LibP2P uses "protocol tables" to agree upon the mapping from one multicodec
   * code. These tables can be application specific, though, like with other
   * multiformats, there is a globally agreed upon table with common protocols
   * and formats.
   */
  class MulticodecType {
   public:
    /// TODO(Harrm) add more codes
    enum Code {
      IDENTITY = 0x00,
      SHA1 = 0x11,
      SHA2_256 = 0x12,
      SHA2_512 = 0x13,
      SHA3_512 = 0x14,
      SHA3_384 = 0x15,
      SHA3_256 = 0x16,
      SHA3_224 = 0x17,
      DAG_PB = 0x70
    };

    static boost::optional<std::string> getName(Code code) {
      using std::string_literals::operator""s;
      switch (code) {
        case Code::IDENTITY:
          return "raw"s;
        case Code::SHA1:
          return "sha1"s;
        case Code::SHA2_256:
          return "sha2-256"s;
        case Code::SHA2_512:
          return "sha2-512"s;
        case Code::SHA3_224:
          return "sha3-224"s;
        case Code::SHA3_256:
          return "sha3-256"s;
        case Code::SHA3_384:
          return "sha3-384"s;
        case Code::SHA3_512:
          return "sha3-512"s;
        case Code::DAG_PB:
          return "dag-pb"s;
      }
      return boost::none;
    }
  };

}  // namespace libp2p::multi

#endif  // LIBP2P_MULTICODECTYPE_HPP
