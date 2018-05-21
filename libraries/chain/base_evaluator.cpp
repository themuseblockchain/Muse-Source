#include <muse/chain/database.hpp>
#include <muse/chain/base_evaluator.hpp>
#include <muse/chain/base_objects.hpp>

#ifndef IS_LOW_MEM
#include <boost/locale/encoding_utf.hpp>

using boost::locale::conv::utf_to_utf;

std::wstring utf8_to_wstring(const std::string& str)
{
    return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

std::string wstring_to_utf8(const std::wstring& str)
{
    return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}

#endif

#include <fc/uint128.hpp>
#include <fc/utf8.hpp>

#include <limits>

namespace muse { namespace chain {
   using fc::uint128_t;

void witness_update_evaluator::do_apply( const witness_update_operation& o )
{
   db().get_account( o.owner ); // verify owner exists

   FC_ASSERT( o.url.size() <= MUSE_MAX_WITNESS_URL_LENGTH );


   const auto& by_witness_name_idx = db().get_index_type< witness_index >().indices().get< by_name >();
   auto wit_itr = by_witness_name_idx.find( o.owner );
   if( wit_itr != by_witness_name_idx.end() )
   {
      db().modify( *wit_itr, [&]( witness_object& w ) {
         w.url                = o.url;
         w.signing_key        = o.block_signing_key;
         w.props              = o.props;
      });
   }
   else
   {
      db().create< witness_object >( [&]( witness_object& w ) {
         w.owner              = o.owner;
         w.url                = o.url;
         w.signing_key        = o.block_signing_key;
         w.created            = db().head_block_time();
         w.props              = o.props;
      });
   }
}


void account_create_evaluator::do_apply( const account_create_operation& o )
{
   if ( o.json_metadata.size() > 0 )
   {
      if( db().has_hardfork( MUSE_HARDFORK_0_3 ) )
         FC_ASSERT( fc::json::is_valid(o.json_metadata), "JSON Metadata not valid JSON" );
      else
         FC_ASSERT( fc::json::is_valid(o.json_metadata, fc::json::broken_nul_parser), "JSON Metadata not valid JSON" );
   }

   if( db().has_hardfork( MUSE_HARDFORK_0_3 ) )
      o.basic.validate();

   const auto& creator = db().get_account( o.creator );

   const auto& props = db().get_dynamic_global_properties();

   if( db().head_block_num() > 0 ){
      FC_ASSERT( creator.balance >= o.fee, "Isufficient balance to create account", ( "creator.balance", creator.balance )( "required", o.fee ) );

      const witness_schedule_object& wso = db().get_witness_schedule_object();
      FC_ASSERT( o.fee >= wso.median_props.account_creation_fee, "Insufficient Fee: ${f} required, ${p} provided",
              ("f", wso.median_props.account_creation_fee)
              ("p", o.fee) );
   }

   db().modify( creator, [&o]( account_object& c ){
      c.balance -= o.fee;
   });

   const auto& new_account = db().create< account_object >( [&o,&props]( account_object& acc )
   {
      acc.name = o.new_account_name;
      acc.owner = o.owner;
      acc.active = o.active;
      acc.basic = o.basic;
      acc.memo_key = o.memo_key;
      acc.last_owner_update = fc::time_point_sec::min();
      acc.created = props.time;
      acc.last_vote_time = props.time;
      acc.mined = false;

      acc.recovery_account = o.creator;

#ifndef IS_LOW_MEM
      acc.json_metadata = o.json_metadata;
#endif
   });

   if( o.fee.amount > 0 )
      db().create_vesting( new_account, o.fee );
}


void account_update_evaluator::do_apply( const account_update_operation& o )
{
   if ( o.json_metadata.size() > 0 )
   {
      if( db().has_hardfork( MUSE_HARDFORK_0_3 ) )
         FC_ASSERT( fc::json::is_valid(o.json_metadata), "JSON Metadata not valid JSON" );
      else
         FC_ASSERT( fc::json::is_valid(o.json_metadata, fc::json::broken_nul_parser), "JSON Metadata not valid JSON" );
   }

   FC_ASSERT( o.account != MUSE_TEMP_ACCOUNT );

   if( db().has_hardfork( MUSE_HARDFORK_0_3 ) && o.basic )
      o.basic->validate();

   const auto& account = db().get_account( o.account );

   if( o.owner )
   {
#ifndef IS_TESTNET
      FC_ASSERT( db().head_block_time() - account.last_owner_update > MUSE_OWNER_UPDATE_LIMIT );
#endif

      db().update_owner_authority( account, *o.owner );
   }

   db().modify( account, [this,&o]( account_object& acc )
   {
      if( o.active ) acc.active = *o.active;
      if( o.basic ) acc.basic = *o.basic;

      if( o.memo_key != public_key_type() )
            acc.memo_key = o.memo_key;

      if( ( o.active || o.owner ) && acc.active_challenged )
      {
         acc.active_challenged = false;
         acc.last_active_proved = db().head_block_time();
      }

#ifndef IS_LOW_MEM
      if ( o.json_metadata.size() > 0 )
         acc.json_metadata = o.json_metadata;
#endif
   });

}

void escrow_transfer_evaluator::do_apply( const escrow_transfer_operation& o ) {
try {
   FC_ASSERT( false, "Escrow transfer operation not enabled" );

   const auto& from_account = db().get_account(o.from);
   db().get_account(o.to);
   const auto& agent_account = db().get_account(o.agent);

   FC_ASSERT( db().get_balance( from_account, o.amount.asset_id ) >= (o.amount + o.fee) );

   if( o.fee.amount > 0 ) {
      db().adjust_balance( from_account, -o.fee );
      db().adjust_balance( agent_account, o.fee );
   }

   db().adjust_balance( from_account, -o.amount );

   db().create<escrow_object>([&]( escrow_object& esc ) {
      esc.escrow_id  = o.escrow_id;
      esc.from       = o.from;
      esc.to         = o.to;
      esc.agent      = o.agent;
      esc.balance    = o.amount;
      esc.expiration = o.expiration;
   });

} FC_CAPTURE_AND_RETHROW( (o) ) }

void escrow_dispute_evaluator::do_apply( const escrow_dispute_operation& o ) {
try {
   FC_ASSERT( false, "Escrow dispute operation not enabled" );
   db().get_account(o.from); // check if it exists

   const auto& e = db().get_escrow( o.from, o.escrow_id );
   FC_ASSERT( !e.disputed );
   FC_ASSERT( e.to == o.to );

   db().modify( e, [&]( escrow_object& esc ){
     esc.disputed = true;
   });
} FC_CAPTURE_AND_RETHROW( (o) ) }

void escrow_release_evaluator::do_apply( const escrow_release_operation& o ) {
try {
   FC_ASSERT( false, "Escrow release operation not enabled" );
   db().get_account(o.from); // check if it exists
   const auto& to_account = db().get_account(o.to);
   db().get_account(o.who); // check if it exists

   const auto& e = db().get_escrow( o.from, o.escrow_id );
   FC_ASSERT( e.balance >= o.amount && e.balance.asset_id == o.amount.asset_id );
   /// TODO assert o.amount > 0

   if( e.expiration > db().head_block_time() ) {
      if( o.who == e.from )    FC_ASSERT( o.to == e.to );
      else if( o.who == e.to ) FC_ASSERT( o.to == e.from );
      else {
         FC_ASSERT( e.disputed && o.who == e.agent );
      }
   } else {
      FC_ASSERT( o.who == e.to || o.who == e.from );
   }

   db().adjust_balance( to_account, o.amount );
   if( e.balance == o.amount )
      db().remove( e );
   else {
      db().modify( e, [&]( escrow_object& esc ) {
         esc.balance -= o.amount;
      });
   }
} FC_CAPTURE_AND_RETHROW( (o) ) }

void transfer_evaluator::do_apply( const transfer_operation& o )
{
   const auto& from_account = db().get_account(o.from);
   const auto& to_account = db().get_account(o.to);

   if( from_account.active_challenged )
   {
      db().modify( from_account, [&]( account_object& a )
      {
         a.active_challenged = false;
         a.last_active_proved = db().head_block_time();
      });
   }

   if( o.amount.asset_id != VESTS_SYMBOL ) {
      FC_ASSERT( db().get_balance( from_account, o.amount.asset_id ) >= o.amount );
      db().adjust_balance( from_account, -o.amount );
      db().adjust_balance( to_account, o.amount );

   } else {
      /// TODO: this line can be removed after hard fork
      FC_ASSERT( false , "transferring of Vestings (VEST) is not allowed." );
#if 0
      /** allow transfer of vesting balance if the full balance is transferred to a new account
       *  This will allow combining of VESTS but not division of VESTS
       **/
      FC_ASSERT( db().get_balance( from_account, o.amount.symbol ) == o.amount );

      db().modify( to_account, [&]( account_object& a ){
          a.vesting_shares += o.amount;
          a.voting_power = std::min( to_account.voting_power, from_account.voting_power );

          // Update to_account bandwidth. from_account bandwidth is already updated as a result of the transfer op
          /*
          auto now = db().head_block_time();
          auto delta_time = (now - a.last_bandwidth_update).to_seconds();
          uint64_t N = trx_size * MUSE_BANDWIDTH_PRECISION;
          if( delta_time >= MUSE_BANDWIDTH_AVERAGE_WINDOW_SECONDS )
             a.average_bandwidth = N;
          else
          {
             auto old_weight = a.average_bandwidth * (MUSE_BANDWIDTH_AVERAGE_WINDOW_SECONDS - delta_time);
             auto new_weight = delta_time * N;
             a.average_bandwidth =  (old_weight + new_weight) / (MUSE_BANDWIDTH_AVERAGE_WINDOW_SECONDS);
          }

          a.average_bandwidth += from_account.average_bandwidth;
          a.last_bandwidth_update = now;
          */

          db().adjust_proxied_witness_votes( a, o.amount.amount, 0 );
      });

      db().modify( from_account, [&]( account_object& a ){
          db().adjust_proxied_witness_votes( a, -o.amount.amount, 0 );
          a.vesting_shares -= o.amount;
      });
#endif
   }
}

void transfer_to_vesting_evaluator::do_apply( const transfer_to_vesting_operation& o )
{
   const auto& from_account = db().get_account(o.from);
   const auto& to_account = o.to.size() ? db().get_account(o.to) : from_account;

   FC_ASSERT( db().get_balance( from_account, MUSE_SYMBOL) >= o.amount );
   db().adjust_balance( from_account, -o.amount );
   db().create_vesting( to_account, o.amount );
}

void withdraw_vesting_evaluator::do_apply( const withdraw_vesting_operation& o )
{
    const auto& account = db().get_account( o.account );

    FC_ASSERT( account.vesting_shares >= asset( 0, VESTS_SYMBOL ) );
    FC_ASSERT( account.vesting_shares >= o.vesting_shares );

    if( !account.mined )  {
      const auto& props = db().get_dynamic_global_properties();
      const witness_schedule_object& wso = db().get_witness_schedule_object();

      asset min_vests = wso.median_props.account_creation_fee * props.get_vesting_share_price();
      min_vests.amount.value *= 10;

      FC_ASSERT( account.vesting_shares > min_vests,
                 "Account registered by another account requires 10x account creation fee worth of Vestings before it can power down" );
    }



    if( o.vesting_shares.amount == 0 ) {
       FC_ASSERT( account.vesting_withdraw_rate.amount  != 0, "this operation would not change the vesting withdraw rate" );

       db().modify( account, [&]( account_object& a ) {
         a.vesting_withdraw_rate = asset( 0, VESTS_SYMBOL );
         a.next_vesting_withdrawal = time_point_sec::maximum();
         a.to_withdraw = 0;
         a.withdrawn = 0;
       });
    }
    else {
       db().modify( account, [&]( account_object& a ) {
         auto new_vesting_withdraw_rate = asset( o.vesting_shares.amount / MUSE_VESTING_WITHDRAW_INTERVALS, VESTS_SYMBOL );

         if( new_vesting_withdraw_rate.amount == 0 )
            new_vesting_withdraw_rate.amount = 1;

         FC_ASSERT( account.vesting_withdraw_rate  != new_vesting_withdraw_rate, "this operation would not change the vesting withdraw rate" );

         a.vesting_withdraw_rate = new_vesting_withdraw_rate;

         a.next_vesting_withdrawal = db().head_block_time() + fc::seconds(MUSE_VESTING_WITHDRAW_INTERVAL_SECONDS);
         a.to_withdraw = o.vesting_shares.amount;
         a.withdrawn = 0;
       });
    }
}

void set_withdraw_vesting_route_evaluator::do_apply( const set_withdraw_vesting_route_operation& o )
{
   try
   {

   const auto& from_account = db().get_account( o.from_account );
   const auto& to_account = db().get_account( o.to_account );
   const auto& wd_idx = db().get_index_type< withdraw_vesting_route_index >().indices().get< by_withdraw_route >();
   auto itr = wd_idx.find( boost::make_tuple( from_account.id, to_account.id ) );

   if( itr == wd_idx.end() )
   {
      FC_ASSERT( o.percent != 0, "Cannot create a 0% destination." );
      FC_ASSERT( from_account.withdraw_routes < MUSE_MAX_WITHDRAW_ROUTES );

      db().create< withdraw_vesting_route_object >( [&]( withdraw_vesting_route_object& wvdo )
      {
         wvdo.from_account = from_account.id;
         wvdo.to_account = to_account.id;
         wvdo.percent = o.percent;
         wvdo.auto_vest = o.auto_vest;
      });

      db().modify( from_account, [&]( account_object& a )
      {
         a.withdraw_routes++;
      });
   }
   else if( o.percent == 0 )
   {
      db().remove( *itr );

      db().modify( from_account, [&]( account_object& a )
      {
         a.withdraw_routes--;
      });
   }
   else
   {
      db().modify( *itr, [&]( withdraw_vesting_route_object& wvdo )
      {
         wvdo.from_account = from_account.id;
         wvdo.to_account = to_account.id;
         wvdo.percent = o.percent;
         wvdo.auto_vest = o.auto_vest;
      });
   }

   itr = wd_idx.upper_bound( boost::make_tuple( from_account.id, account_id_type() ) );
   uint16_t total_percent = 0;

   while( itr->from_account == from_account.id && itr != wd_idx.end() )
   {
      total_percent += itr->percent;
      ++itr;
   }

   FC_ASSERT( total_percent <= MUSE_100_PERCENT, "More than 100% of vesting allocated to destinations" );
   }
   FC_CAPTURE_AND_RETHROW()
}

void account_witness_proxy_evaluator::do_apply( const account_witness_proxy_operation& o )
{
   const auto& account = db().get_account( o.account );
   FC_ASSERT( account.proxy != o.proxy, "something must change" );

   /// remove all current votes
   std::array<share_type, MUSE_MAX_PROXY_RECURSION_DEPTH+1> delta;
   delta[0] = -account.vesting_shares.amount;
   for( int i = 0; i < MUSE_MAX_PROXY_RECURSION_DEPTH; ++i )
      delta[i+1] = -account.proxied_vsf_votes[i];
   db().adjust_proxied_witness_votes( account, delta );

   if( o.proxy.size() ) {
      const auto& new_proxy = db().get_account( o.proxy );
      flat_set<account_id_type> proxy_chain({account.get_id(), new_proxy.get_id()});
      proxy_chain.reserve( MUSE_MAX_PROXY_RECURSION_DEPTH + 1 );

      /// check for proxy loops and fail to update the proxy if it would create a loop
      auto cprox = &new_proxy;
      while( cprox->proxy.size() != 0 ) {
         const auto next_proxy = db().get_account( cprox->proxy );
         FC_ASSERT( proxy_chain.insert( next_proxy.get_id() ).second, "Attempt to create a proxy loop" );
         cprox = &next_proxy;
         FC_ASSERT( proxy_chain.size() <= MUSE_MAX_PROXY_RECURSION_DEPTH, "Proxy chain is too long" );
      }

      /// clear all individual vote records
      db().clear_witness_votes( account );
      db().clear_streaming_platform_votes( account );

      db().modify( account, [&]( account_object& a ) {
         a.proxy = o.proxy;
      });

      /// add all new votes
      for( int i = 0; i <= MUSE_MAX_PROXY_RECURSION_DEPTH; ++i )
         delta[i] = -delta[i];
      db().adjust_proxied_witness_votes( account, delta );
   } else { /// we are clearing the proxy which means we simply update the account
      db().modify( account, [&]( account_object& a ) {
          a.proxy = o.proxy;
      });
   }
}


void account_witness_vote_evaluator::do_apply( const account_witness_vote_operation& o )
{
   const auto& voter = db().get_account( o.account );
   FC_ASSERT( voter.proxy.size() == 0, "A proxy is currently set, please clear the proxy before voting for a witness" );

   const auto& witness = db().get_witness( o.witness );

   const auto& by_account_witness_idx = db().get_index_type< witness_vote_index >().indices().get< by_account_witness >();
   auto itr = by_account_witness_idx.find( boost::make_tuple( voter.get_id(), witness.get_id() ) );

   if( itr == by_account_witness_idx.end() ) {
      FC_ASSERT( o.approve, "vote doesn't exist, user must be indicate a desire to approve witness" );

      FC_ASSERT( voter.witnesses_voted_for < MUSE_MAX_ACCOUNT_WITNESS_VOTES, "account has voted for too many witnesses" ); // TODO: Remove after hardfork 2

      db().create<witness_vote_object>( [&]( witness_vote_object& v ) {
          v.witness = witness.id;
          v.account = voter.id;
      });

      db().adjust_witness_vote( witness, voter.witness_vote_weight() );

      db().modify( voter, [&]( account_object& a ) {
         a.witnesses_voted_for++;
      });

   } else {
      FC_ASSERT( !o.approve, "vote currently exists, user must be indicate a desire to reject witness" );
      db().adjust_witness_vote( witness, -voter.witness_vote_weight() );

      db().modify( voter, [&]( account_object& a ) {
         a.witnesses_voted_for--;
      });
      db().remove( *itr );
   }
}




void custom_evaluator::do_apply( const custom_operation& o ){}

void custom_json_evaluator::do_apply( const custom_json_operation& o )
{
   if ( o.json.size() > 0 )
   {
      if( db().has_hardfork( MUSE_HARDFORK_0_3 ) )
         FC_ASSERT( fc::json::is_valid(o.json), "JSON data not valid JSON" );
      else
         FC_ASSERT( fc::json::is_valid(o.json, fc::json::broken_nul_parser), "JSON data not valid JSON" );
   }

   for( const auto& auth : o.required_basic_auths )
   {
      const auto& acnt = db().get_account( auth );
      FC_ASSERT( !( acnt.owner_challenged || acnt.active_challenged ) );
   }
}

void feed_publish_evaluator::do_apply( const feed_publish_operation& o )
{
  const auto& witness = db().get_witness( o.publisher );
  db().modify( witness, [&]( witness_object& w ){
      w.mbd_exchange_rate = o.exchange_rate;
      w.last_mbd_exchange_update = db().head_block_time();
  });
}

void convert_evaluator::do_apply( const convert_operation& o )
{
  const auto& owner = db().get_account( o.owner );
  FC_ASSERT( db().get_balance( owner, o.amount.asset_id ) >= o.amount );

  db().adjust_balance( owner, -o.amount );

  const auto& fhistory = db().get_feed_history();
  FC_ASSERT( !fhistory.current_median_history.is_null() );

  db().create<convert_request_object>( [&]( convert_request_object& obj )
  {
      obj.owner           = o.owner;
      obj.requestid       = o.requestid;
      obj.amount          = o.amount;
      obj.conversion_date = db().head_block_time() + MUSE_CONVERSION_DELAY; // 1 week
  });

}

void limit_order_create_evaluator::do_apply( const limit_order_create_operation& o )
{
   FC_ASSERT( o.expiration > db().head_block_time() );

   const auto& owner = db().get_account( o.owner );

   FC_ASSERT( db().get_balance( owner, o.amount_to_sell.asset_id ) >= o.amount_to_sell );

   db().adjust_balance( owner, -o.amount_to_sell );

   const auto& order = db().create<limit_order_object>( [&]( limit_order_object& obj )
   {
       obj.created    = db().head_block_time();
       obj.seller     = o.owner;
       obj.orderid    = o.orderid;
       obj.for_sale   = o.amount_to_sell.amount;
       obj.sell_price = o.get_price();
       obj.expiration = o.expiration;
   });

   bool filled = db().apply_order( order );

   if( o.fill_or_kill ) FC_ASSERT( filled );
}

void limit_order_create2_evaluator::do_apply( const limit_order_create2_operation& o )
{
   FC_ASSERT( o.expiration > db().head_block_time() );

   const auto& owner = db().get_account( o.owner );

   FC_ASSERT( db().get_balance( owner, o.amount_to_sell.asset_id ) >= o.amount_to_sell );

   db().adjust_balance( owner, -o.amount_to_sell );

   const auto& order = db().create<limit_order_object>( [&]( limit_order_object& obj )
   {
       obj.created    = db().head_block_time();
       obj.seller     = o.owner;
       obj.orderid    = o.orderid;
       obj.for_sale   = o.amount_to_sell.amount;
       obj.sell_price = o.exchange_rate;
       obj.expiration = o.expiration;
   });

   bool filled = db().apply_order( order );

   if( o.fill_or_kill ) FC_ASSERT( filled );
}

void limit_order_cancel_evaluator::do_apply( const limit_order_cancel_operation& o )
{
   db().cancel_order( db().get_limit_order( o.owner, o.orderid ) );
}

void report_over_production_evaluator::do_apply( const report_over_production_operation& o )
{
   FC_ASSERT( !db().is_producing(), "this operation is currently disabled" );
   FC_ASSERT( false , "this operation is disabled" );

   /*
   const auto& reporter = db().get_account( o.reporter );
   const auto& violator = db().get_account( o.first_block.witness );
   const auto& witness  = db().get_witness( o.first_block.witness );
   FC_ASSERT( violator.vesting_shares.amount > 0, "violator has no vesting shares, must have already been reported" );
   FC_ASSERT( (db().head_block_num() - o.first_block.block_num()) < MUSE_MAX_MINERS, "must report within one round" );
   FC_ASSERT( (db().head_block_num() - witness.last_confirmed_block_num) < MUSE_MAX_MINERS, "must report within one round" );
   FC_ASSERT( public_key_type(o.first_block.signee()) == witness.signing_key );

   db().modify( reporter, [&]( account_object& a ){
       a.vesting_shares += violator.vesting_shares;
       db().adjust_proxied_witness_votes( a, violator.vesting_shares.amount, 0 );
   });
   db().modify( violator, [&]( account_object& a ){
       db().adjust_proxied_witness_votes( a, -a.vesting_shares.amount, 0 );
       a.vesting_shares.amount = 0;
   });
   */
}

void challenge_authority_evaluator::do_apply( const challenge_authority_operation& o )
{
   const auto& challenged = db().get_account( o.challenged );
   const auto& challenger = db().get_account( o.challenger );

   if( o.require_owner )
   {
      FC_ASSERT( false, "Challenging the owner key is not supported at this time" );
#if 0
      FC_ASSERT( challenger.balance >= MUSE_OWNER_CHALLENGE_FEE );
      FC_ASSERT( !challenged.owner_challenged );
      FC_ASSERT( db().head_block_time() - challenged.last_owner_proved > MUSE_OWNER_CHALLENGE_COOLDOWN );

      db().adjust_balance( challenger, - MUSE_OWNER_CHALLENGE_FEE );
      db().create_vesting( db().get_account( o.challenged ), MUSE_OWNER_CHALLENGE_FEE );

      db().modify( challenged, [&]( account_object& a )
      {
         a.owner_challenged = true;
      });
#endif
  }
  else
  {
      FC_ASSERT( challenger.balance >= MUSE_ACTIVE_CHALLENGE_FEE );
      FC_ASSERT( !( challenged.owner_challenged || challenged.active_challenged ) );
      FC_ASSERT( db().head_block_time() - challenged.last_active_proved > MUSE_ACTIVE_CHALLENGE_COOLDOWN );

      db().adjust_balance( challenger, - MUSE_ACTIVE_CHALLENGE_FEE );
      db().create_vesting( db().get_account( o.challenged ), MUSE_ACTIVE_CHALLENGE_FEE );

      db().modify( challenged, [&]( account_object& a )
      {
         a.active_challenged = true;
      });
  }
}

void prove_authority_evaluator::do_apply( const prove_authority_operation& o )
{
   const auto& challenged = db().get_account( o.challenged );
   FC_ASSERT( challenged.owner_challenged || challenged.active_challenged );

   db().modify( challenged, [&]( account_object& a )
   {
      a.active_challenged = false;
      a.last_active_proved = db().head_block_time();
      if( o.require_owner )
      {
         a.owner_challenged = false;
         a.last_owner_proved = db().head_block_time();
      }
   });
}

void request_account_recovery_evaluator::do_apply( const request_account_recovery_operation& o )
{

   const auto& account_to_recover = db().get_account( o.account_to_recover );

   if ( account_to_recover.recovery_account.length() )   // Make sure recovery matches expected recovery account
      FC_ASSERT( account_to_recover.recovery_account == o.recovery_account );
   else                                                  // Empty string recovery account defaults to top witness
      FC_ASSERT( db().get_index_type< witness_index >().indices().get< by_vote_name >().begin()->owner == o.recovery_account );

   const auto& recovery_request_idx = db().get_index_type< account_recovery_request_index >().indices().get< by_account >();
   auto request = recovery_request_idx.find( o.account_to_recover );

   if( request == recovery_request_idx.end() ) // New Request
   {
      FC_ASSERT( !o.new_owner_authority.is_impossible(), "Cannot recover with an impossible authority" );
      FC_ASSERT( o.new_owner_authority.weight_threshold, "Cannot recover with an open authority" );

      // Check accounts in the new authority exist
      db().create< account_recovery_request_object >( [&]( account_recovery_request_object& req )
      {
         req.account_to_recover = o.account_to_recover;
         req.new_owner_authority = o.new_owner_authority;
         req.expires = db().head_block_time() + MUSE_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
      });
   }
   else if( o.new_owner_authority.weight_threshold == 0 ) // Cancel Request if authority is open
   {
      db().remove( *request );
   }
   else // Change Request
   {
      FC_ASSERT( !o.new_owner_authority.is_impossible(), "Cannot recover with an impossible authority" );

      db().modify( *request, [&]( account_recovery_request_object& req )
      {
         req.new_owner_authority = o.new_owner_authority;
         req.expires = db().head_block_time() + MUSE_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
      });
   }
}

void recover_account_evaluator::do_apply( const recover_account_operation& o )
{

   const auto& account = db().get_account( o.account_to_recover );

   FC_ASSERT( db().head_block_time() - account.last_account_recovery > MUSE_OWNER_UPDATE_LIMIT );

   const auto& recovery_request_idx = db().get_index_type< account_recovery_request_index >().indices().get< by_account >();
   auto request = recovery_request_idx.find( o.account_to_recover );

   FC_ASSERT( request != recovery_request_idx.end() );
   FC_ASSERT( request->new_owner_authority == o.new_owner_authority );

   const auto& recent_auth_idx = db().get_index_type< owner_authority_history_index >().indices().get< by_account >();
   auto hist = recent_auth_idx.lower_bound( o.account_to_recover );
   bool found = false;

   while( hist != recent_auth_idx.end() && hist->account == o.account_to_recover && !found )
   {
      found = hist->previous_owner_authority == o.recent_owner_authority;
      if( found ) break;
      ++hist;
   }

   FC_ASSERT( found, "Recent authority not found in authority history" );

   db().remove( *request ); // Remove first, update_owner_authority may invalidate iterator
   db().update_owner_authority( account, o.new_owner_authority );
   db().modify( account, [&]( account_object& a )
   {
      a.last_account_recovery = db().head_block_time();
   });
}

void change_recovery_account_evaluator::do_apply( const change_recovery_account_operation& o )
{

   db().get_account( o.new_recovery_account ); // Simply validate account exists
   const auto& account_to_recover = db().get_account( o.account_to_recover );

   const auto& change_recovery_idx = db().get_index_type< change_recovery_account_request_index >().indices().get< by_account >();
   auto request = change_recovery_idx.find( o.account_to_recover );

   if( request == change_recovery_idx.end() ) // New request
   {
      db().create< change_recovery_account_request_object >( [&]( change_recovery_account_request_object& req )
      {
         req.account_to_recover = o.account_to_recover;
         req.recovery_account = o.new_recovery_account;
         req.effective_on = db().head_block_time() + MUSE_OWNER_AUTH_RECOVERY_PERIOD;
      });
   }
   else if( account_to_recover.recovery_account != o.new_recovery_account ) // Change existing request
   {
      db().modify( *request, [&]( change_recovery_account_request_object& req )
      {
         req.recovery_account = o.new_recovery_account;
         req.effective_on = db().head_block_time() + MUSE_OWNER_AUTH_RECOVERY_PERIOD;
      });
   }
   else // Request exists and changing back to current recovery account
   {
      db().remove( *request );
   }
}

} } // muse::chain
