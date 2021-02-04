#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(protobuf)

libp2p_add_library(p2p_plaintext
    plaintext.cpp
    plaintext_connection.cpp
    )
target_link_libraries(p2p_plaintext
    Boost::boost
    p2p_crypto_error
    p2p_logger
    p2p_plaintext_exchange_message_marshaller
    p2p_protobuf_message_read_writer
    p2p_security_error
    )

libp2p_add_library(p2p_plaintext_exchange_message_marshaller
    exchange_message_marshaller_impl.cpp
    )
target_link_libraries(p2p_plaintext_exchange_message_marshaller
    Boost::boost
    p2p_plaintext_proto
    p2p_key_marshaller
    )
