#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(plaintext_adaptor_test
    plaintext_adaptor_test.cpp
    )
target_link_libraries(plaintext_adaptor_test
    libp2p_plaintext
    buffer
    libp2p_peer_id
    multihash
    multiaddress
    random_generator
    key_generator
    key_marshaller
    )

addtest(plaintext_exchange_message_marshaller_test
    plaintext_exchange_message_marshaller_test.cpp
    )
target_link_libraries(plaintext_exchange_message_marshaller_test
    buffer
    libp2p_peer_id
    multihash
    multiaddress
    plaintext_exchange_message_marshaller
)
