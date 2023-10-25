/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/crypto/hmac_provider/hmac_provider_ctr_impl.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::security::noise, HKDFError, e) {
  using E = libp2p::security::noise::HKDFError;
  switch (e) {
    case E::ILLEGAL_OUTPUTS_NUMBER:
      return "Noise HKDF() may produce one, two, or three outputs only";
  }
  return "unknown";
}

namespace libp2p::security::noise {

  using HMAC = crypto::hmac::HmacProviderCtrImpl;

  outcome::result<HKDFResult> hkdf(HashType hash_type,
                                   size_t outputs,
                                   BytesIn chaining_key,
                                   BytesIn input_key_material) {
    if (0 == outputs or outputs > 3) {
      return HKDFError::ILLEGAL_OUTPUTS_NUMBER;
    }
    HKDFResult result;

    HMAC temp_mac{hash_type, spanToVec(chaining_key)};
    OUTCOME_TRY(temp_mac.write(input_key_material));
    OUTCOME_TRY(temp_key, temp_mac.digest());

    HMAC out1_mac{hash_type, temp_key};
    Bytes one(1, 0x01);
    OUTCOME_TRY(out1_mac.write(one));
    OUTCOME_TRY(out1, out1_mac.digest());
    result.one = out1;
    if (1 == outputs) {
      return result;
    }

    HMAC out2_mac{hash_type, temp_key};
    OUTCOME_TRY(out2_mac.write(out1));
    Bytes two(1, 0x02);
    OUTCOME_TRY(out2_mac.write(two));
    OUTCOME_TRY(out2, out2_mac.digest());
    result.two = out2;
    if (2 == outputs) {
      return result;
    }

    HMAC out3_mac{hash_type, temp_key};
    OUTCOME_TRY(out3_mac.write(out2));
    Bytes three(1, 0x03);
    OUTCOME_TRY(out3_mac.write(three));
    OUTCOME_TRY(out3, out3_mac.digest());
    result.three = out3;

    return result;
  }

}  // namespace libp2p::security::noise
