#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(utils)


addtest(multihash_test
    multihash_test.cpp
    )
target_link_libraries(multihash_test
    multihash
    )

addtest(multistream_test
    multistream_test.cpp
    )
target_link_libraries(multistream_test
    multistream
    )

addtest(multiaddress_test
    multiaddress_test.cpp
    )
target_link_libraries(multiaddress_test
    multiaddress
    buffer
    )

addtest(multibase_codec_test
    multibase_codec_test.cpp
    )
target_link_libraries(multibase_codec_test
    multibase_codec
    )
