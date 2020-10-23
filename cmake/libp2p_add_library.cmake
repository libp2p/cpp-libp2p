function(libp2p_add_library target)
  add_library(${target}
      ${ARGN}
      )
  target_include_directories(${target}
    INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/libs/libp2p/include>
    $<INSTALL_INTERFACE:libp2p/include>
  )
  libp2p_install(${target})
endfunction()
