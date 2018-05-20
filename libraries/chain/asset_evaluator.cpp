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
#include <muse/chain/base_evaluator.hpp>
#include <muse/chain/asset_object.hpp>
#include <muse/chain/account_object.hpp>
#include <muse/chain/database.hpp>
#include <muse/chain/exceptions.hpp>
#include <muse/chain/hardfork.hpp>

#include <functional>

namespace muse { namespace chain {


bool _is_authorized_asset( const database& d, const account_object& acct, const asset_object& asset_obj )
{
    return true; //TODO_MUSE - check asset trading restriction
}


void asset_create_evaluator::do_apply( const asset_create_operation& op )
{ try {
   //TODO_MUSE - add fee
   auto& asset_indx = db().get_index_type<asset_index>().indices().get<by_symbol>();
   auto asset_symbol_itr = asset_indx.find( op.symbol );
   FC_ASSERT( asset_symbol_itr == asset_indx.end(), "Asset with symbol ${s} already exist", ("s",op.symbol) );
  
   account_object issuer=db().get_account(op.issuer);
   auto dotpos = op.symbol.rfind( '.' );
   if( dotpos != std::string::npos )
   {
      auto prefix = op.symbol.substr( 0, dotpos );
      auto asset_symbol_itr = asset_indx.find( prefix );
      FC_ASSERT( asset_symbol_itr != asset_indx.end(), "Asset ${s} may only be created by issuer of ${p}, but ${p} has not been registered",
                 ("s",op.symbol)("p",prefix) );
      FC_ASSERT( asset_symbol_itr->issuer == issuer.id, "Asset ${s} may only be created by issuer of ${p}",
                 ("s",op.symbol)("p",prefix) );
   }

   db().create<asset_object>( [&issuer,&op]( asset_object& a ) {
      a.issuer = issuer.id;
      a.symbol_string = op.symbol;
      a.precision = op.precision;
      a.options = op.common_options;
      a.current_supply = 0;
   });

   return; 
} FC_CAPTURE_AND_RETHROW( (op) ) }

void asset_issue_evaluator::do_apply( const asset_issue_operation& o )
{ try {
   auto& asset_indx = db().get_index_type<asset_index>().indices().get<by_id>();
   auto asset_symbol_itr = asset_indx.find( o.asset_to_issue.asset_id );
   FC_ASSERT( asset_symbol_itr != asset_indx.end(), "Asset with symbol id ${d} does not exist exist", ("d",o.asset_to_issue.asset_id) );
   FC_ASSERT( db().get_account(o.issuer).id == asset_symbol_itr -> issuer );
   FC_ASSERT( _is_authorized_asset( db(), db().get_account(o.issue_to_account), *asset_symbol_itr ) );
   FC_ASSERT( (asset_symbol_itr -> current_supply + o.asset_to_issue.amount) <= asset_symbol_itr -> options.max_supply );

   db().adjust_balance( db().get_account(o.issue_to_account), o.asset_to_issue );
   db().modify( *asset_symbol_itr, [&]( asset_object& data ){
        data.current_supply += o.asset_to_issue.amount;
   });

   return ;
} FC_CAPTURE_AND_RETHROW( (o) ) }

void asset_reserve_evaluator::do_apply( const asset_reserve_operation& o )
{ try {
   auto& asset_indx = db().get_index_type<asset_index>().indices().get<by_id>();
   auto asset_symbol_itr = asset_indx.find( o.amount_to_reserve.asset_id );
   FC_ASSERT( asset_symbol_itr != asset_indx.end(), "Asset with symbol id ${d} does not exist exist", ("d",o.amount_to_reserve.asset_id) );

   FC_ASSERT( _is_authorized_asset( db(), db().get_account(o.payer), *asset_symbol_itr ) );
   db().adjust_balance( db().get_account(o.payer), -o.amount_to_reserve );
   db().modify( *asset_symbol_itr, [&]( asset_object& data ){
        data.current_supply -= o.amount_to_reserve.amount;
   });

   return ;
} FC_CAPTURE_AND_RETHROW( (o) ) }

void asset_update_evaluator::do_apply(const asset_update_operation& o)
{ try {
   database& d = db();

   auto& asset_indx = db().get_index_type<asset_index>().indices().get<by_id>();
   auto asset_symbol_itr = asset_indx.find( o.asset_to_update );
   FC_ASSERT( asset_symbol_itr != asset_indx.end(), "Asset with symbol id ${d} does not exist exist", ("d",o.asset_to_update) );

   auto a = *asset_symbol_itr;
   auto a_copy = a; 
   a_copy.options = o.new_options;
   a_copy.validate();

   if( o.new_issuer )
   {
       d.get_account(*o.new_issuer);
   }

   FC_ASSERT(!(o.new_options.issuer_permissions & ~a.options.issuer_permissions),
             "Cannot reinstate previously revoked issuer permissions on an asset.");

   // changed flags must be subset of old issuer permissions
   FC_ASSERT(!((o.new_options.flags ^ a.options.flags) & ~a.options.issuer_permissions),
             "Flag change is forbidden by issuer permissions");

   
   FC_ASSERT( d.get_account(o.issuer).id == a.issuer, "", ("o.issuer", (d.get_account(o.issuer).id))("a.issuer", a.issuer) ) ;


   db().modify( *asset_symbol_itr, [&](asset_object& a) {
      if( o.new_issuer )
         a.issuer = d.get_account(*o.new_issuer).id;
      a.options = o.new_options;
   });
   return ;
} FC_CAPTURE_AND_RETHROW((o)) }

} } // muse::chain
