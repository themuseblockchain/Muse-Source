file(GLOB HEADERS "include/muse/app/*.hpp")
file(GLOB EGENESIS_HEADERS "../egenesis/include/muse/egenesis/*.hpp")

add_library( muse_app
             database_api.cpp
             api.cpp
             application.cpp
             impacted.cpp
             plugin.cpp
             ${HEADERS}
             ${EGENESIS_HEADERS}
           )

target_link_libraries( muse_app muse_chain  muse_mf_plugins fc graphene_db graphene_net graphene_utilities )
target_include_directories( muse_app
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" 
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../egenesis/include")

if(MSVC)
  set_source_files_properties( application.cpp api.cpp database_api.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   muse_app

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
