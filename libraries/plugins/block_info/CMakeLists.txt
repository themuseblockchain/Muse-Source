file(GLOB HEADERS "include/muse/plugins/block_info/*.hpp")

add_library( muse_block_info
             ${HEADERS}
             block_info_plugin.cpp
             block_info_api.cpp
           )

target_link_libraries( muse_block_info muse_app muse_chain fc graphene_db )
target_include_directories( muse_block_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
