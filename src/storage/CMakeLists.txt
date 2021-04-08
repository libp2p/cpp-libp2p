#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_sqlite sqlite.cpp)
target_link_libraries(p2p_sqlite
    SQLiteModernCpp::SQLiteModernCpp
    p2p_logger
    )
