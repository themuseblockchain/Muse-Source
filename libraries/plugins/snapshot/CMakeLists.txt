file(GLOB HEADERS "include/graphene/snapshot/*.hpp")

add_library( muse_snapshot
             snapshot.cpp
           )

target_link_libraries( muse_snapshot muse_chain muse_app )
target_include_directories( muse_snapshot
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   muse_snapshot

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
