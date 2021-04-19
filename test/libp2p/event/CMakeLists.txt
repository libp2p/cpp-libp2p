#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(event_emitter_test
    event_emitter_test.cpp
    )

target_link_libraries(event_emitter_test
    p2p_logger
    )

addtest(event_bus_test
    event_bus_test.cpp
    )

target_link_libraries(event_bus_test
    p2p_logger
    )
