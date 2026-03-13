cmake_minimum_required( VERSION 3.15 )

project( uda_data_source )

set( SOURCES
    src/uda_data_source.cpp
)

add_library( json_data_source SHARED data_source_library/json_data_source.cpp )
target_link_libraries( json_data_source PRIVATE LibTokaMap::libtokamap )
