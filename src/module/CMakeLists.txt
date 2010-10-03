set ( MODULE_NAME eater )
set ( MODULE_FILE ${MODULE_NAME}.ko )

function ( sources_to_objects var )
  foreach ( file ${ARGN} )
    get_filename_component ( path ${file} PATH )
    get_filename_component ( name ${file} NAME_WE )

    if ( path )
      set ( objs ${objs} ${path}/${name}.o )
    else ()
      set ( objs ${objs} ${name}.o )
    endif ( path )

  endforeach ( file )

  set ( ${var} ${objs} CACHE INTERNAL "" FORCE )
endfunction ( sources_to_objects )

function ( space_separated var )
  foreach ( item ${ARGN} )
    set ( str "${str} ${item}" )
  endforeach ( item )

  set ( ${var} ${str} CACHE INTERNAL "" FORCE )
endfunction ( space_separated )

function ( to_include_param var )
  foreach ( dir ${ARGN} )
    set ( str "${str} -I${dir}" )
  endforeach ( dir )

  set ( ${var} ${str} CACHE INTERNAL "" FORCE )
endfunction ( to_include_param )




file ( GLOB_RECURSE MODULE_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h )
file ( GLOB_RECURSE MODULE_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c )
set  ( MODULE_ALL_SOURCES ${MODULE_HEADERS} ${MODULE_SOURCES} )

set ( KBUILD_SOURCES ${MODULE_SOURCES} utils/log2.c )
sources_to_objects ( KBUILD_OBJECTS ${KBUILD_SOURCES} )
space_separated    ( KBUILD_OBJECTS_STR ${KBUILD_OBJECTS} )

set ( MODULE_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} )
include_directories ( ${MODULE_OUTPUT_DIR} )

get_directory_property ( INCLUDE_DIRECTORIES INCLUDE_DIRECTORIES )
to_include_param ( INCLUDE_DIRECTORIES_PARAM ${INCLUDE_DIRECTORIES} )

set (
  KBUILD_COMMAND
  ${CMAKE_MAKE_PROGRAM} ARCH=${ARCH}
                        -C ${KERNEL_DIR} M=${MODULE_OUTPUT_DIR}
                        EXTRA_CFLAGS=${INCLUDE_DIRECTORIES_PARAM}
                        modules
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
  DEPENDS ${MODULE_SOURCES} ${MODULE_HEADERS}
          ${MODULE_OUTPUT_DIR}/utils/log2_table.inc Kbuild.in
  VERBATIM
)

set ( LOG_ARG_MULTIPLIER    255 )
set ( LOG_RESULT_MULTIPLIER 10000 )

set ( GENERATE_LOG2_TABLE_COMMAND
  seq 1 ${LOG_ARG_MULTIPLIER} |
  xargs -I{}
    echo "l({} / ${LOG_ARG_MULTIPLIER}) / l(2) * ${LOG_RESULT_MULTIPLIER}" |
    bc -l |
    sed "s/\\..*$/,/" > log2_table.inc
)

add_custom_command (
  OUTPUT ${MODULE_OUTPUT_DIR}/utils/log2_table.inc
  COMMAND ${GENERATE_LOG2_TABLE_COMMAND}
  WORKING_DIRECTORY ${MODULE_OUTPUT_DIR}/utils
  VERBATIM
)

configure_file (
  utils/log2.h.in
  ${MODULE_OUTPUT_DIR}/utils/log2.h
)

configure_file (
  utils/log2.c.in
  ${MODULE_OUTPUT_DIR}/utils/log2.c
)

add_custom_target (
  module
  ALL
  DEPENDS ${MODULE_OUTPUT_DIR}/${MODULE_FILE}
)
