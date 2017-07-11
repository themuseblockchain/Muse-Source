#pragma once
#include <muse/chain/protocol/operations.hpp>
#include <muse/chain/evaluator.hpp>
#include <muse/chain/database.hpp>

namespace muse { namespace chain {

   class custom_evaluator : public evaluator<custom_evaluator>
   {
      public:
         typedef custom_operation operation_type;

         void do_evaluate( const custom_operation& o ){}
         void do_apply( const custom_operation& o ){}
   };
} }
