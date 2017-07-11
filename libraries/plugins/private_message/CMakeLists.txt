file(GLOB HEADERS "include/muse/private_message/*.hpp")

add_library( muse_private_message
             private_message_plugin.cpp
           )

target_link_libraries( muse_private_message muse_chain muse_app graphene_time )
target_include_directories( muse_private_message
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   muse_private_message

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
