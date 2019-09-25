function(print)
  message(STATUS "[${CMAKE_PROJECT_NAME}] ${ARGV}")
endfunction()

function(fatal_error)
  message(FATAL_ERROOR "[${CMAKE_PROJECT_NAME}] ${ARGV}")
endfunction()

