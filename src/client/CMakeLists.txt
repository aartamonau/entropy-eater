file ( GLOB EATER_CLIENT_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c )

include_directories ( ${EATER_LIB_SOURCE_DIR} )

set ( CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -std=gnu99 )
add_executable ( eater_client ${EATER_CLIENT_SOURCES} )
target_link_libraries ( eater_client eater )
