#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(protobuf)

libp2p_add_library(p2p_identify
    identify.cpp
    identify_msg_processor.cpp
    observed_addresses.cpp
    identify_push.cpp
    identify_delta.cpp
    utils.cpp
    )
target_link_libraries(p2p_identify
    p2p
    p2p_identify_proto
    p2p_protobuf_message_read_writer
    p2p_logger
    )
