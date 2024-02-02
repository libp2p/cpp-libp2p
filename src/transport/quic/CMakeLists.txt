#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

set( QUIC_SOURCES
quic_transport.cpp quic_stream.cpp quic_listener.cpp quic_connection.cpp 
)

libp2p_add_library(p2p_quic SHARED ${QUIC_SOURCES})
target_link_libraries(p2p_quic  PUBLIC  
    p2p_multiaddress
    p2p_logger
    p2p_connection_error
    ssl::ssl # BoringSSL
    nexus    
)
# prevent the openssl/ssl.h header from ever being read if it is on the include path
# because Hunter uses the same common path for all packages i.e.
#/home/vagrant/.hunter/_Base/72b446a/b18b06c/eede853/Install/include ....
#                                                                   /openssl
#                                                                   ...
#                                                                 /boringssl
# see compile_commands.json to see what i mean
# BoringSSL's ssl.h has OPENSSL_HEADER_SSL_H as include guard
#     
# target_compile_definitions(p2p_quic PRIVATE 
# OPENSSL_SSL_H 
# HEADER_SSL_H
#  ) 

# get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
#get_target_property(dirs p2p_quic INCLUDE_DIRECTORIES )
#foreach(dir ${dirs})
#  message(STATUS "dir='${dir}'")
#endforeach()

# do not leak any BoringSSL symbols
# or boost::asio::ssl symbols that use the boringssl headers
# instead of the openssl ones in their definition
target_compile_options( p2p_quic PRIVATE
-fvisibility=hidden 
-fvisibility-inlines-hidden 
)

# is nexus imported by executable or build itself
if( TARGET p2p_quic )
target_compile_definitions( p2p_quic PUBLIC
 $<BUILD_INTERFACE:LIBP2P_QUIC_DLL_EXPORTS>
$<INSTALL_INTERFACE:> )
endif()    
