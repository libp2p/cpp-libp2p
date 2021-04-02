#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(p2p_clock
    impl/clock_impl.cpp
    )

add_library(p2p_async_testutil
    manual_scheduler_backend.cpp
    )
