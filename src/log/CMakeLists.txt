#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_logger
    logger.cpp
    configurator.cpp
    )
target_link_libraries(p2p_logger
    soralog::soralog
    soralog::fallback_configurator
    soralog::configurator_yaml
    )
