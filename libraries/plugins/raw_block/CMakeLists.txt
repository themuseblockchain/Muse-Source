file(GLOB HEADERS "include/muse/plugins/raw_block/*.hpp")

add_library( muse_raw_block
             ${HEADERS}
             raw_block_plugin.cpp
             raw_block_api.cpp
           )

target_link_libraries( muse_raw_block muse_app muse_chain fc graphene_db )
target_include_directories( muse_raw_block
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
