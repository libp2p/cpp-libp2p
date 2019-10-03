function(libp2p_add_library target)
  add_library(${target}
      ${ARGN}
      )
  libp2p_install(${target})
endfunction()
