file(GLOB HEADERS "include/muse/witness/*.hpp")

add_library( muse_witness
             witness.cpp
           )

target_link_libraries( muse_witness muse_chain muse_app )
target_include_directories( muse_witness
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   muse_witness

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
