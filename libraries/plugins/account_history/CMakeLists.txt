file(GLOB HEADERS "include/muse/account_history/*.hpp")

add_library( muse_account_history
             account_history_plugin.cpp
           )

target_link_libraries( muse_account_history muse_chain muse_app )
target_include_directories( muse_account_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   muse_account_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
