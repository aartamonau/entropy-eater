function ( subdir_variables prefix subdir )
  set ( ${prefix}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}
        CACHE INTERNAL "" FORCE )
  set ( ${prefix}_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${subdir}
        CACHE INTERNAL "" FORCE )
endfunction (subdir_variables )

subdir_variables ( EATER_MODULE module )
subdir_variables ( EATER_LIB    lib )
subdir_variables ( EATER_CLIENT client )

add_subdirectory ( module )
add_subdirectory ( lib )
add_subdirectory ( client )
