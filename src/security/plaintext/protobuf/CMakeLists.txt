#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_proto_library(p2p_plaintext_proto
    plaintext.proto
    )
target_link_libraries(p2p_plaintext_proto
    p2p_keys_proto
    )
