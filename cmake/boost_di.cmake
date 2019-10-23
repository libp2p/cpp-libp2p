#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

#######################################
# load and set up boost di dependency #
#######################################

include(FeatureSummary)
include(ExternalProject)

add_library(di INTERFACE IMPORTED)

set(GIT_URL https://github.com/boost-experimental/di.git)
set(BOOTS_DI_VERSION cpp14)
set(GIT_TAG "cpp14")

set_package_properties(boost_di
    PROPERTIES
    URL ${GIT_URL}
    DESCRIPTION "boost experimental dependency injection library"
    )

externalproject_add(boost_di
    GIT_REPOSITORY  ${GIT_URL}
    GIT_TAG         ${GIT_TAG}
    GIT_SHALLOW     1
    TLS_VERIFY true
    INSTALL_COMMAND "" # remove install step
    )

add_dependencies(di boost_di)

externalproject_get_property(boost_di source_dir)
set(boost_di_INCLUDE_DIR ${source_dir}/include ${source_dir}/extension/include)
file(MAKE_DIRECTORY ${boost_di_INCLUDE_DIR})

target_include_directories(di INTERFACE ${boost_di_INCLUDE_DIR})

