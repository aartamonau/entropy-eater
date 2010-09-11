set ( MODULE_NAME eater )
set ( MODULE_FILE ${MODULE_NAME}.ko )

function ( sources_to_objects var )
  foreach ( file ${ARGN} )
    get_filename_component ( name ${file} NAME_WE )
    set ( objs ${objs} ${name}.o )
  endforeach ( file )

  set ( ${var} ${objs} CACHE INTERNAL "" FORCE )
endfunction ( sources_to_objects )

function ( space_separated var )
  foreach ( item ${ARGN} )
    set ( str "${str} ${item}" )
  endforeach ( item )

  set ( ${var} ${str} CACHE INTERNAL "" FORCE )
endfunction ( space_separated )

file ( GLOB MODULE_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h )
file ( GLOB MODULE_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c )
set  ( MODULE_ALL_SOURCES ${MODULE_HEADERS} ${MODULE_SOURCES} )

sources_to_objects ( MODULE_OBJECTS ${MODULE_SOURCES} )
space_separated    ( MODULE_OBJECTS_STR ${MODULE_OBJECTS} )

set ( MODULE_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} )

set (
  KBUILD_COMMAND
  ${CMAKE_MAKE_PROGRAM} ARCH=${ARCH}
                        -C ${KERNEL_DIR} M=${MODULE_OUTPUT_DIR} modules
)

configure_file (
  Kbuild.in
  ${MODULE_OUTPUT_DIR}/Kbuild
)

foreach ( source ${MODULE_ALL_SOURCES} )
  configure_file (
    ${source}
    ${MODULE_OUTPUT_DIR}/${source}
    COPYONLY
  )
endforeach ( source )

add_custom_command (
  OUTPUT ${MODULE_OUTPUT_DIR}/${MODULE_FILE}
  COMMAND ${KBUILD_COMMAND}
  WORKING_DIRECTORY ${MODULE_OUTPUT_DIR}
  DEPENDS ${MODULE_SOURCES} ${MODULE_HEADERS} Kbuild.in
  VERBATIM
)

add_custom_target (
  module
  ALL
  DEPENDS ${MODULE_OUTPUT_DIR}/${MODULE_FILE}
)
