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

add_library( json_data_source SHARED ${SOURCES} )
target_include_directories( json_data_source PRIVATE ${UDA_INCLUDE_DIRS} )
target_link_directories( json_data_source PRIVATE ${UDA_LIBRARY_DIRS} )
target_link_libraries( json_data_source PRIVATE
    LibTokaMap::libtokamap
    ${UDA_CLIENT_LIBRARIES}
    ${UDA_PLUGINS_LIBRARIES}
)
