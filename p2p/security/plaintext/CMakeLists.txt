#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(protobuf)

add_library(libp2p_plaintext
    plaintext.cpp
    plaintext.hpp
    plaintext_connection.cpp
    plaintext_connection.hpp
    plaintext_session.cpp
    plaintext_session.hpp
    )
target_link_libraries(libp2p_plaintext
    outcome
    libp2p_security_error
    plaintext_exchange_message_marshaller
    crypto_error
    )

add_library(plaintext_exchange_message_marshaller
    exchange_message_marshaller_impl.hpp
    exchange_message_marshaller_impl.cpp
    )
target_link_libraries(plaintext_exchange_message_marshaller
    outcome
    plaintext_protobuf
    key_marshaller
    )
