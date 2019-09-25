#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(multiselect
    multiselect.cpp
    multiselect.hpp
    message_manager.cpp
    message_manager.hpp
    message_reader.cpp
    message_reader.hpp
    message_writer.cpp
    message_writer.hpp
    multiselect_error.cpp
    multiselect_error.hpp
    )
target_link_libraries(multiselect
    uvarint
    multihash
    logger
    )
