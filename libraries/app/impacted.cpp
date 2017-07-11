/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <muse/chain/protocol/authority.hpp>
#include <muse/app/impacted.hpp>

#include <fc/utility.hpp>

namespace muse { namespace app {

using namespace fc;
using namespace muse::chain;

// TODO:  Review all of these, especially no-ops
struct get_impacted_account_visitor
{
   flat_set< string >& _impacted;
   get_impacted_account_visitor( flat_set< string >& impact ):_impacted( impact ) {}
   typedef void result_type;

   template<typename T>
   void operator()( const T& op )
   {
      op.get_required_active_authorities( _impacted );
      op.get_required_owner_authorities( _impacted );
   }

   void operator()( const account_create_operation& op )
   {
      _impacted.insert( op.new_account_name );
      _impacted.insert( op.creator );
   }

   void operator()( const account_update_operation& op )
   {
      _impacted.insert( op.account );
   }


   void operator()( const vote_operation& op )
   {
      _impacted.insert( op.voter );
   }


   void operator()( const curate_reward_operation& op )
   {
      _impacted.insert( op.curator );
   }

   void operator()( const content_reward_operation& op )
   {
      _impacted.insert( op.payee );
   }

   void operator()( const playing_reward_operation& op )
   {
      _impacted.insert( op.platform );
   }

   void operator()( const liquidity_reward_operation& op )
   {
      _impacted.insert( op.owner );
   }

   void operator()( const interest_operation& op )
   {
      _impacted.insert( op.owner );
   }

   void operator()( const fill_convert_request_operation& op )
   {
      _impacted.insert( op.owner );
   }

   void operator()( const transfer_operation& op )
   {
      _impacted.insert( op.from );
      _impacted.insert( op.to );
   }

   void operator()( const transfer_to_vesting_operation& op )
   {
      _impacted.insert( op.from );

      if ( !op.to.empty() && op.to != op.from )
      {
         _impacted.insert( op.to );
      }
   }

   void operator()( const withdraw_vesting_operation& op )
   {
      _impacted.insert( op.account );
   }

   void operator()( const witness_update_operation& op )
   {
      _impacted.insert( op.owner );
   }

   void operator()( const account_witness_vote_operation& op )
   {
      _impacted.insert( op.account );
      _impacted.insert( op.witness );
   }

   void operator()( const account_witness_proxy_operation& op )
   {
      _impacted.insert( op.account );
      _impacted.insert( op.proxy );
   }

   void operator()( const feed_publish_operation& op )
   {
      _impacted.insert( op.publisher );
   }

   void operator()( const limit_order_create_operation& op )
   {
      _impacted.insert( op.owner );
   }

   void operator()( const fill_order_operation& op )
   {
      _impacted.insert( op.current_owner );
      _impacted.insert( op.open_owner );
   }

   void operator()( const limit_order_cancel_operation& op )
   {
      _impacted.insert( op.owner );
   }

   void operator()( const fill_vesting_withdraw_operation& op )
   {
      _impacted.insert( op.from_account );
      _impacted.insert( op.to_account );
   }

   void operator()( const custom_operation& op )
   {
      for( auto s: op.required_auths )
         _impacted.insert( s );
   }

   void operator()( const request_account_recovery_operation& op )
   {
      _impacted.insert( op.account_to_recover );
   }

   void operator()( const recover_account_operation& op )
   {
      _impacted.insert( op.account_to_recover );
   }

   void operator()( const change_recovery_account_operation& op )
   {
      _impacted.insert( op.account_to_recover );
   }

   void operator()( const streaming_platform_update_operation& op )
   {
      _impacted.insert( op.owner );
   }

   void operator()( const account_streaming_platform_vote_operation& op )
   {
      _impacted.insert( op.account );
      _impacted.insert( op.streaming_platform );
   }

   void operator()( const streaming_platform_report_operation& op )
   {
      _impacted.insert( op.streaming_platform );
      _impacted.insert( op.consumer );
   }

   void operator()( const friendship_operation& op )
   {
      _impacted.insert( op.who );
      _impacted.insert( op.whom );
   }

   void operator()( const unfriend_operation& op )
   {
      _impacted.insert( op.who );
      _impacted.insert( op.whom );
   }

   void operator()( const content_operation& op )
   {
      _impacted.insert( op.uploader );
      for (const auto d: op.distributions )
         _impacted.insert( d.payee );
      if(op.distributions_comp)
         for (const auto d: *op.distributions_comp )
            _impacted.insert( d.payee );
      for (const auto m: op.management )
         _impacted.insert( m.voter );
      if(op.management_comp)
         for (const auto m: *op.management_comp )
            _impacted.insert( m.voter );
   }

   void operator()( const content_update_operation& op )
   {
      for (const auto d: op.new_distributions )
         _impacted.insert( d.payee );

      for (const auto m: op.new_management )
         _impacted.insert( m.voter );
   }

   void operator()( const content_approve_operation& op )
   {
      _impacted.insert( op.approver );
   }
   //void operator()( const operation& op ){}
};

void operation_get_impacted_accounts( const operation& op, flat_set< string >& result )
{
   get_impacted_account_visitor vtor = get_impacted_account_visitor( result );
   op.visit( vtor );
}

void transaction_get_impacted_accounts( const transaction& tx, flat_set< string >& result )
{
   for( const auto& op : tx.operations )
      operation_get_impacted_accounts( op, result );
}

} }
