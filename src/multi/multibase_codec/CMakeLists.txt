#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multibase_codec
    multibase_codec_impl.cpp
    codecs/base16.cpp
    codecs/base58.cpp
    codecs/base64.cpp
    codecs/base_error.cpp
    )
target_link_libraries(multibase_codec
    hexutil
    Boost::boost
    )
