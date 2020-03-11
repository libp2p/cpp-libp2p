#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_multibase_codec
    multibase_codec_impl.cpp
    codecs/base16.cpp
    codecs/base32.cpp
    codecs/base58.cpp
    codecs/base64.cpp
    codecs/base_error.cpp
    )
target_link_libraries(p2p_multibase_codec
    p2p_hexutil
    Boost::boost
    )
