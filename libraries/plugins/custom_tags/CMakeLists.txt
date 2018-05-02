file(GLOB HEADERS "include/muse/custom_tags/*.hpp")

add_library( muse_custom_tags
             custom_tags.cpp
             custom_tags_api.cpp
           )

target_link_libraries( muse_custom_tags muse_chain muse_app )
target_include_directories( muse_custom_tags
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   muse_custom_tags

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
