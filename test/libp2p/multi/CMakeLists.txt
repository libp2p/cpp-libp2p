#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(utils)


addtest(multihash_test
    multihash_test.cpp
    )
target_link_libraries(multihash_test
    p2p_multihash
    p2p_literals
    )

addtest(multiaddress_test
    multiaddress_test.cpp
    )
target_link_libraries(multiaddress_test
    p2p_multiaddress
    p2p_literals
    )

addtest(multibase_codec_test
    multibase_codec_test.cpp
    )
target_link_libraries(multibase_codec_test
    p2p_multibase_codec
    p2p_literals
    )

addtest(cid_test
    cid_test.cpp
    )
target_link_libraries(cid_test
    p2p_cid
    p2p_multibase_codec
    p2p_literals
    )
