#include <muse/chain/protocol/base_operations.hpp>
#include <fc/io/json.hpp>

#include <locale>
#include <boost/algorithm/string/predicate.hpp>

namespace muse { namespace chain {

   inline void validate_url(const string& url )
   {
      FC_ASSERT(url.size() < MUSE_MAX_URL_LENGTH );
      FC_ASSERT(boost::starts_with( url, "ipfs://" ) );
            //TODO_MUSE - more checks...
   }

   bool inline is_asset_type( asset asset, asset_id_type asset_id )
   {
      return asset.asset_id == asset_id;
   }

   void account_create_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( new_account_name ), "Invalid account name" );
      FC_ASSERT( is_asset_type( fee, MUSE_SYMBOL ), "Account creation fee must be MUSE" );
      owner.validate();
      active.validate();
      basic.validate();

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         //FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
      FC_ASSERT( fee >= asset( 0, MUSE_SYMBOL ) );
   }

   void account_update_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( account ) );
      if( owner )
         owner->validate();
      if (active)
         active->validate();
      if( basic )
         basic->validate();

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         //FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
   }

   void challenge_authority_operation::validate()const
    {
      FC_ASSERT( is_valid_account_name( challenger ), "challenger account name invalid" );
      FC_ASSERT( is_valid_account_name( challenged ), "challenged account name invalid" );
      FC_ASSERT( challenged != challenger );
   }

   void prove_authority_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( challenged ), "challenged account name invalid" );
   }


   void transfer_operation::validate() const
   { try {
      FC_ASSERT( is_valid_account_name( from ), "Invalid 'from' account name" );
      FC_ASSERT( is_valid_account_name( to ), "Invalid 'to' account name" );
      FC_ASSERT( amount.amount > 0, "Cannot transfer a negative amount (aka: stealing)" );
      FC_ASSERT( memo.size() < MUSE_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   } FC_CAPTURE_AND_RETHROW( (*this) ) }

   void transfer_to_vesting_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( from ), "From account name invalid" );
      FC_ASSERT( is_asset_type( amount, MUSE_SYMBOL ), "Amount must be MUSE" );
      if ( !to.empty() ) FC_ASSERT( is_valid_account_name( to ), "To account name invalid" );
      FC_ASSERT( amount > asset( 0, MUSE_SYMBOL ), "Must transfer a nonzero amount" );
   }

   void withdraw_vesting_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( account ), "Account name invalid" );
      FC_ASSERT( is_asset_type( vesting_shares, VESTS_SYMBOL), "Amount must be VESTS"  );
      //FC_ASSERT( vesting_shares.amount >= 0, "Cannot withdraw a negative amount of VESTS!" );
   }

   void set_withdraw_vesting_route_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( from_account ), "Account name invalid" );
      FC_ASSERT( is_valid_account_name( to_account ), "Account name invalid" );
      FC_ASSERT( 0 <= percent && percent <= MUSE_100_PERCENT, "Percent must be valid muse percent" );
   }

   void witness_update_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( owner ), "Owner account name invalid" );
      FC_ASSERT( url.size() > 0, "URL size must be greater than 0" );
      FC_ASSERT( fc::is_utf8( url ) );
      FC_ASSERT( fee >= asset( 0, MUSE_SYMBOL ) );
      props.validate();
   }

   void account_witness_vote_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( account ), "Account ${a}", ("a",account)  );
      FC_ASSERT( is_valid_account_name( witness ) );
   }

   void account_witness_proxy_operation::validate() const
   {
      FC_ASSERT( is_valid_account_name( account ) );
      if( proxy.size() )
         FC_ASSERT( is_valid_account_name( proxy ) );
      FC_ASSERT( proxy != account );
   }



   void custom_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( required_auths.size() > 0, "at least on account must be specified" );
   }
   void custom_json_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( (required_auths.size() + required_basic_auths.size()) > 0, "at least on account must be specified" );
      FC_ASSERT( id.size() <= 32 );
      FC_ASSERT( fc::is_utf8(json), "JSON Metadata not formatted in UTF8" );
      //FC_ASSERT( fc::json::is_valid(json), "JSON Metadata not valid JSON" );
   }

   void feed_publish_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( publisher ) );
      FC_ASSERT( ( is_asset_type( exchange_rate.base, MUSE_SYMBOL ) && is_asset_type( exchange_rate.quote, MBD_SYMBOL ) )
         || ( is_asset_type( exchange_rate.base, MBD_SYMBOL ) && is_asset_type( exchange_rate.quote, MUSE_SYMBOL ) ) );
      exchange_rate.validate();
   }

   void limit_order_create_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( owner ) );
      FC_ASSERT ( is_asset_type( amount_to_sell, MBD_SYMBOL ) ||  is_asset_type( min_to_receive, MBD_SYMBOL ) );
      FC_ASSERT ( !is_asset_type( min_to_receive, VESTS_SYMBOL ) && !is_asset_type( amount_to_sell, VESTS_SYMBOL ) );
      (amount_to_sell / min_to_receive).validate();
   }
   void limit_order_create2_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( owner ) );
      FC_ASSERT( amount_to_sell.asset_id == exchange_rate.base.asset_id );
      exchange_rate.validate();

      FC_ASSERT ( is_asset_type( exchange_rate.base, MBD_SYMBOL ) ||  is_asset_type( exchange_rate.quote, MBD_SYMBOL ) );
      FC_ASSERT ( !is_asset_type( exchange_rate.base, VESTS_SYMBOL ) && !is_asset_type( exchange_rate.quote, VESTS_SYMBOL ) );

      FC_ASSERT( (amount_to_sell * exchange_rate).amount > 0 ); // must not round to 0
   }

   void limit_order_cancel_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( owner ) );
   }

   void convert_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( owner ) );
      /// only allow conversion from MBD to MUSE, allowing the opposite can enable traders to abuse
      /// market fluxuations through converting large quantities without moving the price.
      FC_ASSERT( is_asset_type( amount, MBD_SYMBOL ) );
      FC_ASSERT( amount.amount > 0 );
   }

   void report_over_production_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( reporter ) );
      FC_ASSERT( is_valid_account_name( first_block.witness ) );
      FC_ASSERT( first_block.witness   == second_block.witness );
      FC_ASSERT( first_block.timestamp == second_block.timestamp );
      FC_ASSERT( first_block.signee()  == second_block.signee() );
      FC_ASSERT( first_block.id() != second_block.id() );
   }

   void escrow_transfer_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( from ) );
      FC_ASSERT( is_valid_account_name( to ) );
      FC_ASSERT( is_valid_account_name( agent ) );
      FC_ASSERT( fee.amount >= 0 );
      FC_ASSERT( amount.amount >= 0 );
      FC_ASSERT( from != agent && to != agent );
      FC_ASSERT( fee.asset_id == amount.asset_id );
      FC_ASSERT( amount.asset_id != VESTS_SYMBOL );
   }

   void escrow_dispute_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( from ) );
      FC_ASSERT( is_valid_account_name( to ) );
      FC_ASSERT( is_valid_account_name( who ) );
      FC_ASSERT( who == from || who == to );
   }

   void escrow_release_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( from ) );
      FC_ASSERT( is_valid_account_name( to ) );
      FC_ASSERT( is_valid_account_name( who ) );
      FC_ASSERT( amount.amount > 0 );
      FC_ASSERT( amount.asset_id != VESTS_SYMBOL );
   }

   void request_account_recovery_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( recovery_account ) );
      FC_ASSERT( is_valid_account_name( account_to_recover ) );
      new_owner_authority.validate();
   }

   void recover_account_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( account_to_recover ) );
      FC_ASSERT( !( new_owner_authority == recent_owner_authority) );
      FC_ASSERT( !new_owner_authority.is_impossible() );
      FC_ASSERT( !recent_owner_authority.is_impossible() );
      FC_ASSERT( new_owner_authority.weight_threshold );
      new_owner_authority.validate();
      recent_owner_authority.validate();
   }

   void change_recovery_account_operation::validate()const
   {
      FC_ASSERT( is_valid_account_name( account_to_recover ) );
      FC_ASSERT( is_valid_account_name( new_recovery_account ) );
   }


} } // muse::chain
