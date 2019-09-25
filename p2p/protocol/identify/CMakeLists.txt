#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(protobuf)

add_library(libp2p_identify
    identify.cpp
    identify.hpp
    identify_msg_processor.cpp
    identify_msg_processor.hpp
    observed_addresses.cpp
    observed_addresses.hpp
    identify_push.cpp
    identify_push.hpp
    identify_delta.cpp
    identify_delta.hpp
    utils.cpp
    utils.hpp
    )
target_link_libraries(libp2p_identify
    libp2p
    identify_proto
    protobuf_message_read_writer
    )
