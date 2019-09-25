#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(libp2p_yamux
    yamux.cpp
    yamux.hpp
    )
target_link_libraries(libp2p_yamux
    libp2p_yamuxed_connection
    )


add_library(libp2p_yamuxed_connection
    yamuxed_connection.cpp
    yamuxed_connection.hpp
    yamux_frame.cpp
    yamux_frame.hpp
    yamux_stream.cpp
    yamux_stream.hpp
    )
target_link_libraries(libp2p_yamuxed_connection
    outcome
    logger
    )
