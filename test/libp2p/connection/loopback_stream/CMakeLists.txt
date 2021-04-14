#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(loopback_stream_test
    loopback_stream_test.cpp
    )
target_link_libraries(loopback_stream_test
    p2p_loopback_stream
    )
