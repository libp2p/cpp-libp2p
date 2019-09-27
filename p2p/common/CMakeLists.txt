#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(hexutil
    hexutil.hpp
    hexutil.cpp
    )
target_link_libraries(hexutil
    Boost::boost
    outcome
    )

add_library(logger
    logger.cpp
    )
