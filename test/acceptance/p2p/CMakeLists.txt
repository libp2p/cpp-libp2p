#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(host)

addtest(all_muxers_acceptance_test
    muxer.cpp
    )
target_link_libraries(all_muxers_acceptance_test
    libp2p_yamux
    multiaddress
    libp2p_plaintext
    libp2p_tcp
    testutil
    key_marshaller
    libp2p_identity_manager
    )
