# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

libp2p_add_library(scheduler
    scheduler.cpp
    )
target_link_libraries(scheduler
    Boost::boost
    p2p_logger
    )

libp2p_add_library(asio_scheduler
    asio/asio_scheduler.cpp
    )
target_link_libraries(asio_scheduler
    scheduler
    )

libp2p_add_library(subscription
    subscription.cpp
    )

