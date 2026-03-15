cmake_minimum_required( VERSION 3.15 )

project( uda_data_source )

# Specify C++ standard for all targets
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

# Extra cmake modules to find libraries
list(
    APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/cmake/Modules
)

find_package( LibTokaMap REQUIRED )
find_package( UDA REQUIRED )

set( SOURCES
    src/uda_data_source.cpp
)

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    set( LIBRARY_NAME_SUFFIX "_debug" )
else()
    set( LIBRARY_NAME_SUFFIX "" )
endif()

set( LIBRARY_NAME "uda_data_source${LIBRARY_NAME_SUFFIX}" )

add_library( ${LIBRARY_NAME} SHARED ${SOURCES} )
target_include_directories( ${LIBRARY_NAME} PRIVATE ${UDA_INCLUDE_DIRS} )
target_link_directories( ${LIBRARY_NAME} PRIVATE ${UDA_LIBRARY_DIRS} )
target_link_libraries( ${LIBRARY_NAME} PRIVATE
    LibTokaMap::libtokamap
    ${UDA_CLIENT_LIBRARIES}
)

include( GNUInstallDirs )

install( TARGETS ${LIBRARY_NAME}
    EXPORT ${LIBRARY_NAME}Targets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
