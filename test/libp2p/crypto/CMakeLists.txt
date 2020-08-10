#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(keys_test
    keys_test.cpp
    )

addtest(aes_test
    aes_test.cpp
    )
target_link_libraries(aes_test
    p2p_aes_provider
    p2p_testutil
    p2p_literals
    )

addtest(chachapoly_test
    chachapoly_test.cpp
    )
target_link_libraries(chachapoly_test
    p2p_chachapoly_provider
    p2p_testutil
    p2p_literals
    )

addtest(ecdsa_test
    ecdsa_provider_test.cpp
    )
target_link_libraries(ecdsa_test
    p2p_ecdsa_provider
    )

addtest(hmac_test
    hmac_test.cpp
    )
target_link_libraries(hmac_test
    p2p_hmac_provider
    p2p_testutil
    p2p_literals
    )

addtest(random_test
    random_test.cpp
    )
target_link_libraries(random_test
    p2p_random_generator
    )

addtest(marshaller_test
    marshaller_test.cpp
    )
target_link_libraries(marshaller_test
    p2p_key_marshaller
    )

addtest(keygen_test
    key_generator_test.cpp
    )
target_link_libraries(keygen_test
    p2p_crypto_provider
    p2p_literals
    )

addtest(key_validator_test
    key_validator_test.cpp
    )
target_link_libraries(key_validator_test
    p2p_key_validator
    )

addtest(rsa_provider_test
    rsa_provider_test.cpp
    )
target_link_libraries(rsa_provider_test
    p2p_rsa_provider
    )

addtest(secp256k1_test
    secp256k1_test.cpp
    )
target_link_libraries(secp256k1_test
    p2p_secp256k1_provider
    p2p_multibase_codec
    )

addtest(x25519_test
    x25519_provider_test.cpp
    )
target_link_libraries(x25519_test
    p2p_x25519_provider
    p2p_literals
    )
