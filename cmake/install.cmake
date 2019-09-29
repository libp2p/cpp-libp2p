#function(install_deps_headers)
#  install(DIRECTORY ${MICROSOFT.GSL_ROOT}/include/gsl
#      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
#
#  install(DIRECTORY deps/outcome/outcome
#      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
#
#endfunction()
#
#function(libp2p_install)
#  set(options)
#  set(oneValueArgs)
#  set(multiValueArgs TARGETS HEADER_DIRS)
#  cmake_parse_arguments(arg "${options}" "${oneValueArgs}"
#      "${multiValueArgs}" ${ARGN})
#
#  install(TARGETS ${arg_TARGETS} EXPORT libp2pTargets
#      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#      PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#      FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX}
#      )
#
#  foreach (DIR IN ITEMS ${arg_HEADER_DIRS})
#    get_filename_component(FULL_PATH ${DIR} ABSOLUTE)
#    string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/p2p libp2p RELATIVE_PATH ${FULL_PATH})
#    get_filename_component(INSTALL_PREFIX ${RELATIVE_PATH} DIRECTORY)
#    install(DIRECTORY ${DIR}
#        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${INSTALL_PREFIX}
#        FILES_MATCHING PATTERN "*.hpp")
#  endforeach ()
#
#  install(
#      EXPORT libp2pTargets
#      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libp2p
#      NAMESPACE libp2p::
#  )
#  export(
#      EXPORT libp2pTargets
#      FILE ${CMAKE_CURRENT_BINARY_DIR}/libp2pTargets.cmake
#      NAMESPACE libp2p::
#  )
#
#endfunction()

include(GNUInstallDirs)
set(TARGETS_TO_BE_INSTALLED "")
function (libp2p_install targets)
  install(TARGETS ${targets} EXPORT libp2pConfig
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX}
      )
  list(APPEND TARGETS_TO_BE_INSTALLED ${targets})
endfunction()

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/libp2p
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(
    EXPORT libp2pConfig
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libp2p
    NAMESPACE p2p::
)
#export(
#    TARGETS ${TARGETS_TO_BE_INSTALLED}
#    FILE libp2pConfig.cmake
#)
