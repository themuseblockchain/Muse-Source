file(GLOB HEADERS "include/muse/plugins/debug_node/*.hpp")

add_library( muse_debug_node
             ${HEADERS}
             debug_node_plugin.cpp
             debug_node_api.cpp
           )

target_link_libraries( muse_debug_node muse_app muse_chain fc graphene_db )
target_include_directories( muse_debug_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
