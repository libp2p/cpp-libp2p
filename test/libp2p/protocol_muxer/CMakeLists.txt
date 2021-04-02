#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(multiselect_test
    multiselect_test.cpp
    )
target_link_libraries(multiselect_test
    p2p_multiselect
    p2p_multiaddress
    p2p_peer_id
    p2p_yamux
    p2p_tcp
    p2p_plaintext
    p2p_testutil_peer
    p2p_literals
    )
