#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if (TESTING)
  # https://docs.hunter.sh/en/latest/packages/pkg/GTest.html
  hunter_add_package(GTest)
  find_package(GTest CONFIG REQUIRED)
endif()
#############################################################################
#############################################################################

set(OPEN_SSL_INSTALL /usr/local/openssl )



set(OPEN_SSL_LIB /usr/local/openssl/lib64/libssl.so )


set( OPEN_SSL_INCLUDE_DIR /usr/local/openssl/include)

message(STATUS "openssl: ${OPEN_SSL_INCLUDE_DIR}")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    OpenSSL::SSL
    "found_ssl"
    OPEN_SSL_LIB
    OPEN_SSL_INCLUDE_DIR
)

mark_as_advanced( OPEN_SSL_INCLUDE_DIR OPEN_SSL_LIB)

if(NOT TARGET OpenSSL::SSL )
 add_library( OpenSSL::SSL SHARED IMPORTED GLOBAL)

 set_target_properties( OpenSSL::SSL PROPERTIES
 IMPORTED_LOCATION "${OPEN_SSL_LIB}"
 INTERFACE_INCLUDE_DIRECTORIES "${OPEN_SSL_INCLUDE_DIR}"
  IMPORTED_LINK_INTERFACE_LIBRARIES OpenSSL::Crypto
 IMPORTED_LINK_INTERFACE_LANGUAGES "CXX" 
 )
endif()


############################################



set(OPENSSL_CRYPTO_LIB /usr/local/openssl/lib64/libcrypto.so )


#find_path( OPENSSL_CRYPTO_INCLUDE_DIR 
#NAMES openssl/ssl.h
#HINTS "${OPEN_SSL_INSTALL}/include"
#)

find_package_handle_standard_args(
    OpenSSL::Crypto
    DEFAULT_MSG
    OPENSSL_CRYPTO_LIB
   # OPENSLL_CRYPTO_INCLUDE_DIR
    OPEN_SSL_INCLUDE_DIR
)

mark_as_advanced( OPENSSL_CRYPTO_LIB)

if(NOT TARGET OpenSSL::Crypto )
 add_library( OpenSSL::Crypto STATIC IMPORTED GLOBAL)
 set_target_properties( OpenSSL::Crypto PROPERTIES
 IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIB}"
 INTERFACE_INCLUDE_DIRECTORIES "${OPEN_SSL_INCLUDE_DIR}"
 IMPORTED_LINK_INTERFACE_LANGUAGES "CXX" 
 )
endif()


#####################################################################
#####################################################################


#find_package(ZLIB)

set(BOR_SSL_INSTALL /home/vagrant/boringssl/install-static-rel )

#find_library(SSL_LIB NAMES ssl HINTS /usr/local/boringssl/lib )
#set(SSL_LIB /usr/local/boringssl/lib/libssl.so )
#set(SSL_LIB ${BOR_SSL_INSTALL}/lib/libssl.a)
set(SSL_LIB /home/vagrant/boringssl/build-static-rel/ssl/libssl.a ) # build with -fPIC


#find_path( SSL_INCLUDE_DIR 
#NAMES openssl/ssl.h
##HINTS "/usr/local/boringssl/include"
#HINTS "${BOR_SSL_INSTALL}/include"
#)

set(SSL_INCLUDE_DIR ${BOR_SSL_INSTALL}/include )

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    ssl
    DEFAULT_MSG
    SSL_LIB
    SSL_INCLUDE_DIR
)

mark_as_advanced( SSL_INCLUDE_DIR SSL_LIB)

if(NOT TARGET ssl::ssl )
 add_library( ssl::ssl STATIC IMPORTED GLOBAL)

 set_target_properties( ssl::ssl PROPERTIES
 IMPORTED_LOCATION "${SSL_LIB}"
 INTERFACE_INCLUDE_DIRECTORIES "${SSL_INCLUDE_DIR}"
  IMPORTED_LINK_INTERFACE_LIBRARIES crypto::crypto
 IMPORTED_LINK_INTERFACE_LANGUAGES "CXX" 
 )
endif()
############################################


#set(CRYPTO_LIB ${BOR_SSL_INSTALL}/lib/libcrypto.a)
set(CRYPTO_LIB /home/vagrant/boringssl/build-static-rel/crypto/libcrypto.a ) # build with -fPIC


#find_path( CRYPTO_INCLUDE_DIR 
#NAMES openssl/ssl.h
#HINTS "${BOR_SSL_INSTALL}/include"
#)

find_package_handle_standard_args(
    crypto
    DEFAULT_MSG
    CRYPTO_LIB
    SSL_INCLUDE_DIR
)

mark_as_advanced( SSL_INCLUDE_DIR CRYPTO_LIB)

if(NOT TARGET crypto::crypto )
 add_library( crypto::crypto STATIC IMPORTED GLOBAL)
 set_target_properties( crypto::crypto PROPERTIES
 IMPORTED_LOCATION "${CRYPTO_LIB}"
 INTERFACE_INCLUDE_DIRECTORIES "${SSL_INCLUDE_DIR}"
 IMPORTED_LINK_INTERFACE_LANGUAGES "CXX" 
 )
endif()
##############################################

add_library( lsquic STATIC IMPORTED GLOBAL)

#find_library(LSQUIC_LIB NAMES lsquic HINTS /usr/local/lib)


find_path( LSQUIC_INCLUDE_DIR 
NAMES lsquic/lsquic.h
# HINTS /usr/local/include
HINTS /home/vagrant/lsquic/install-debug/include
)
#set(LSQUIC_LIB /usr/local/lib/liblsquic.so )


#set(LSQUIC_LIB /home/vagrant/lsquic/install-debug/lib/liblsquic.so )
set(LSQUIC_LIB /home/vagrant/lsquic/install-static-rel/lib/liblsquic.a )

find_package_handle_standard_args(
    lsquic
    DEFAULT_MSG
    LSQUIC_LIB
    LSQUIC_INCLUDE_DIR
)

mark_as_advanced( LSQUIC_INCLUDE_DIR LSQUIC_LIB)


 #set_target_properties( lsquic PROPERTIES
 #IMPORTED_LOCATION "${LSQUIC_LIB}" 
 #IMPORTED_LINK_INTERFACE_LIBRARIES "$<LINK_GROUP:RESCAN,crypto::crypto,ssl::ssl,ZLIB::ZLIB>" 
 #INTERFACE_INCLUDE_DIRECTORIES "${LSQUIC_INCLUDE_DIR}"
 #IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
 #)


#######################################################################################
#list(APPEND CMAKE_MODULE_PATH /usr/local/lib/cmake/nexus )
#find_package(nexus MODULE REQUIRED)

# https://github.com/amdfxlucas/nexus.git 'dev' branch
# build with -fvisibility-hidden and -fvisibility-inlines-hidden 
# to not leak any BoringSSL symbols 
# or boost::asio::ssl symbols that use the boringssl headers
# instead of the openssl ones in their definition
add_library( nexus SHARED IMPORTED GLOBAL)

#find_library(NEXUS_LIB NAMES nexus HINTS /usr/local/lib)


find_path( NEXUS_INCLUDE_DIR 
NAMES nexus/nexus.hpp
#HINTS /usr/local/include
HINTS /home/vagrant/nexus-bugfix/include
)

#set(NEXUS_LIB /usr/local/lib/nexus/libnexus.so )
#set(NEXUS_LIB /home/vagrant/nexus-bugfix/build-static-rel/src/libnexus.a )

#set(NEXUS_LIB /home/vagrant/nexus-bugfix/build/src/libnexus.so )
set(NEXUS_LIB /home/vagrant/nexus-bugfix/build-shared-deb/src/libnexus.so )

find_package_handle_standard_args(
    nexus
    DEFAULT_MSG
    NEXUS_LIB
    NEXUS_INCLUDE_DIR
)

mark_as_advanced( NEXUS_INCLUDE_DIR NEXUS_LIB)
 set_target_properties( nexus PROPERTIES
 IMPORTED_LOCATION "${NEXUS_LIB}"
 # IMPORTED_LINK_INTERFACE_LIBRARIES "$<LINK_GROUP:RESCAN,lsquic,crypto::crypto,ssl::ssl,ZLIB::ZLIB>" 
 INTERFACE_INCLUDE_DIRECTORIES "${NEXUS_INCLUDE_DIR}"
 IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
 )


###########################################################################

# openssl system installation
#set(OpenSSl_DIR /usr/local/boringssl/lib/cmake/OpenSSL )
#list(APPEND CMAKE_PREFIX_PATH /usr/local/boringssl/lib/cmake/OpenSSL )
#find_package(OpenSSL CONFIG REQUIRED)
#find_package( OpenSSL PATHS .. NO_DEFAULT_PATH)
#-----------------------------------------------------------------------------
#include(/home/vagrant/boringssl/install-debug/lib/cmake/OpenSSL/OpenSSLConfig.cmake )


#######################################################################
# https://docs.hunter.sh/en/latest/packages/pkg/Boost.html
hunter_add_package(Boost COMPONENTS random filesystem program_options )
find_package(Boost CONFIG REQUIRED random filesystem program_options )


# openSSL includes cannot stay on Hunter include path
# i.e. ~/.hunter/_Base/72b446a/b18b06c/eede853/Install/include / openssl
# where p2p_quic target could find them and wrongfully use them,
# instead of the boringssl headers
# https://www.openssl.org/
#set(HUNTER_OpenSSL_CMAKE_ARGS BUILD_SHARED_LIBS=1  ...other options ...) # applied only to OpenSSL package
#hunter_add_package(OpenSSL)
#find_package(OpenSSL REQUIRED)

# required to build nexus
#hunter_add_package(BoringSSL)
#find_package( BoringSSL REQUIRED)

# https://developers.google.com/protocol-buffers/
hunter_add_package(Protobuf)
find_package(Protobuf CONFIG REQUIRED)

find_package(Threads)

hunter_add_package(c-ares)
find_package(c-ares CONFIG REQUIRED)

hunter_add_package(fmt)
find_package(fmt CONFIG REQUIRED)

hunter_add_package(yaml-cpp)
find_package(yaml-cpp CONFIG REQUIRED)

hunter_add_package(soralog)
find_package(soralog CONFIG REQUIRED)

# https://github.com/masterjedy/hat-trie
hunter_add_package(tsl_hat_trie)
find_package(tsl_hat_trie CONFIG REQUIRED)

# https://github.com/masterjedy/di
hunter_add_package(Boost.DI)
find_package(Boost.DI CONFIG REQUIRED)

# https://github.com/qdrvm/libp2p-sqlite-modern-cpp/tree/hunter
hunter_add_package(SQLiteModernCpp)
find_package(SQLiteModernCpp CONFIG REQUIRED)
