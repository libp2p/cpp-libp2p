#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

libp2p_add_library(p2p_multiselect
    multiselect.cpp
    message_manager.cpp
    message_reader.cpp
    message_writer.cpp
    multiselect_error.cpp
    )
target_link_libraries(p2p_multiselect
    p2p_uvarint
    p2p_multihash
    p2p_logger
    )
