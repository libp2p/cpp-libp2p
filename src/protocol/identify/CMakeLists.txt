#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(protobuf)

add_library(libp2p_identify
    identify.cpp
    identify_msg_processor.cpp
    observed_addresses.cpp
    identify_push.cpp
    identify_delta.cpp
    utils.cpp
    )
target_link_libraries(libp2p_identify
    libp2p
    identify_proto
    protobuf_message_read_writer
    )
