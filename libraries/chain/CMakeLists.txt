file(GLOB HEADERS "include/muse/chain/*.hpp" "include/muse/chain/protocol/*.hpp" )

add_custom_target( build_hardfork_hpp
   COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../../programs/build_helpers/cat_parts.py" "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" "${CMAKE_CURRENT_BINARY_DIR}/include/muse/chain/hardfork.hpp" )
set_source_files_properties( "${CMAKE_CURRENT_BINARY_DIR}/include/muse/chain/hardfork.hpp" PROPERTIES GENERATED TRUE )

## SORT .cpp by most likely to change / break compile
add_library( muse_chain

             # As database takes the longest to compile, start it first
             database.cpp
             fork_database.cpp

             protocol/types.cpp
             protocol/authority.cpp
             protocol/operations.cpp
             protocol/sign_state.cpp
             protocol/base_operations.cpp
             protocol/muse_operations.cpp
             protocol/asset.cpp
             protocol/asset_ops.cpp
             protocol/transaction.cpp
             protocol/block.cpp
             protocol/asset.cpp
             protocol/proposal.cpp
             protocol/version.cpp

             get_config.cpp

             evaluator.cpp
             base_evaluator.cpp
             muse_evaluator.cpp
             muse_objects.cpp

             asset_evaluator.cpp
             asset_object.cpp
             proposal_object.cpp
             proposal_evaluator.cpp
             base_objects.cpp
             block_database.cpp

             ${HEADERS}
             "${CMAKE_CURRENT_BINARY_DIR}/include/muse/chain/hardfork.hpp"
        protocol/address.cpp protocol/pts_address.cpp)

add_dependencies( muse_chain build_hardfork_hpp )
target_link_libraries( muse_chain fc graphene_db ${PATCH_MERGE_LIB} )
target_include_directories( muse_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if(MSVC)
  set_source_files_properties( database.cpp block_database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   muse_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
