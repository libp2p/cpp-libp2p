#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_hexutil
    hexutil.cpp
    )
target_link_libraries(p2p_hexutil
    Boost::boost
    )

libp2p_add_library(p2p_logger
    logger.cpp
    )
