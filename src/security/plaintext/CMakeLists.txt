#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(protobuf)

add_library(p2p_plaintext
    plaintext.cpp
    plaintext_connection.cpp
    plaintext_session.cpp
    )
target_link_libraries(p2p_plaintext
    outcome
    p2p_security_error
    p2p_plaintext_exchange_message_marshaller
    p2p_crypto_error
    )

add_library(p2p_plaintext_exchange_message_marshaller
    exchange_message_marshaller_impl.cpp
    )
target_link_libraries(p2p_plaintext_exchange_message_marshaller
    outcome
    p2p_plaintext_protobuf
    p2p_key_marshaller
    )
