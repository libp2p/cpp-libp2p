#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(p2p_hexutil
    hexutil.cpp
    )
target_link_libraries(p2p_hexutil
    Boost::boost
    outcome
    )

add_library(p2p_logger
    logger.cpp
    )
