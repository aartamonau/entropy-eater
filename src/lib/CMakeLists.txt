file ( GLOB EATER_LIB_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c )

include_directories( ${EATER_MODULE_SOURCE_DIR} )
add_library ( eater ${EATER_LIB_SOURCES} )
target_link_libraries ( eater nl )
