
add_library( muse_egenesis_full
   egenesis_full.cpp
   include/muse/egenesis/egenesis.hpp
   )

target_link_libraries( muse_egenesis_full muse_chain fc )
target_include_directories( muse_egenesis_full
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_executable( embed_genesis
   embed_genesis.cpp
   )

target_link_libraries( embed_genesis muse_chain muse_app muse_egenesis_full fc )

INSTALL( TARGETS
   embed_genesis muse_egenesis_full

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
