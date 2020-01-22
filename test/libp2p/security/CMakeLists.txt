#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(plaintext_adaptor_test
    plaintext_adaptor_test.cpp
    )
target_link_libraries(plaintext_adaptor_test
    Boost::Boost.DI
    p2p_plaintext
    p2p_peer_id
    p2p_multihash
    p2p_multiaddress
    p2p_random_generator
    p2p_crypto_provider
    p2p_key_marshaller
    )

addtest(plaintext_exchange_message_marshaller_test
    plaintext_exchange_message_marshaller_test.cpp
    )
target_link_libraries(plaintext_exchange_message_marshaller_test
    p2p_peer_id
    p2p_multihash
    p2p_multiaddress
    p2p_plaintext_exchange_message_marshaller
    )

addtest(secio_exchange_message_marshaller_test
    secio_exchange_message_marshaller_test.cpp
    )
target_link_libraries(secio_exchange_message_marshaller_test
    p2p_secio_exchange_message_marshaller
    )

addtest(secio_propose_message_marshaller_test
    secio_propose_message_marshaller_test.cpp
    )
target_link_libraries(secio_propose_message_marshaller_test
    p2p_secio_propose_message_marshaller
    )
