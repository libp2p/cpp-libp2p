#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multiselect
    multiselect.cpp
    message_manager.cpp
    message_reader.cpp
    message_writer.cpp
    multiselect_error.cpp
    )
target_link_libraries(multiselect
    uvarint
    multihash
    logger
    )
