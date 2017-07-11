file(GLOB HEADERS "include/muse/market_history/*.hpp")

add_library( muse_market_history
             market_history_plugin.cpp
             market_history_api.cpp
           )

target_link_libraries( muse_market_history muse_chain muse_app )
target_include_directories( muse_market_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   muse_market_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)