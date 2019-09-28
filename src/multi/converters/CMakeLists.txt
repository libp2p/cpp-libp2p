#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(converter_utils
    converter_utils.cpp
    )

target_link_libraries(converter_utils
    conversion_error
    uvarint
    hexutil
    converters
    )

add_library(conversion_error
    conversion_error.cpp
    )
target_link_libraries(conversion_error
    outcome
    )

add_library(converters
    ip_v4_converter.cpp
    ip_v6_converter.cpp
    tcp_converter.cpp
    udp_converter.cpp
    ipfs_converter.cpp
    )

target_link_libraries(converters
    outcome
    hexutil
    uvarint
    multibase_codec
    )
