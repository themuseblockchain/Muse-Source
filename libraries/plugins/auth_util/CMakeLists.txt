file(GLOB HEADERS "include/muse/plugins/auth_util/*.hpp")

add_library( muse_auth_util
             ${HEADERS}
             auth_util_plugin.cpp
             auth_util_api.cpp
           )

target_link_libraries( muse_auth_util muse_app muse_chain fc graphene_db )
target_include_directories( muse_auth_util
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
