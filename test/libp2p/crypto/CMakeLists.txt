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
