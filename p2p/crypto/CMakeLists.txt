#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(aes_provider)
add_subdirectory(hmac_provider)
add_subdirectory(key_generator)
add_subdirectory(key_marshaller)
add_subdirectory(key_validator)
add_subdirectory(protobuf)
add_subdirectory(random_generator)
add_subdirectory(sha)

add_library(crypto_error
    error.hpp
    error.cpp
    )
target_link_libraries(crypto_error
    outcome
    )

add_library(crypto_key
    key.cpp
    key.hpp
    )
target_link_libraries(crypto_key
    Boost::boost
    )
