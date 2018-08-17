#include <muse/app/api_context.hpp>
#include <muse/app/application.hpp>
#include <muse/app/database_api.hpp>
#include <muse/chain/get_config.hpp>
#include <muse/chain/base_objects.hpp>
#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>

#include <fc/crypto/hex.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>


#include <cctype>

#include <cfenv>
#include <iostream>
#include <locale>

#define GET_REQUIRED_FEES_MAX_RECURSION 4

static const std::locale& c_locale = std::locale::classic();

namespace muse { namespace app {

class database_api_impl : public std::enable_shared_from_this<database_api_impl>
{
   public:
      explicit database_api_impl( muse::chain::database& db );
      ~database_api_impl();

      // Objects
      fc::variants get_objects(const vector<object_id_type>& ids)const;

      // Subscriptions
      void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );
      void cancel_all_subscriptions();

      // Blocks and transactions
      optional<block_header> get_block_header(uint32_t block_num)const;
      optional<signed_block> get_block(uint32_t block_num)const;
      vector<proposal_object> get_proposed_transactions( string id )const;

      // Globals
      fc::variant_object get_config()const;
      dynamic_global_property_object get_dynamic_global_properties()const;

      // Keys
      vector<set<string>> get_key_references( vector<public_key_type> key )const;

      // Accounts
      vector< extended_account > get_accounts( const vector< string >& names )const;
      optional< account_object > get_account_from_id( account_id_type account_id )const;
      vector<account_id_type> get_account_references( account_id_type account_id )const;
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;
      set<string> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_account_count()const;

      // Witnesses
      vector<optional<witness_object>> get_witnesses(const vector<witness_id_type>& witness_ids)const;
      fc::optional<witness_object> get_witness_by_account( string account_name )const;
      set<string> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_witness_count()const;
     

      // Streamin platforms
      set<string> lookup_streaming_platform_accounts(const string& lower_bound_name, uint32_t limit)const;
      bool is_streaming_platform(string straming_platform)const;
      //content
      vector<report_object> get_reports_for_account(string consumer)const;
      vector<content_object> get_content_by_uploader(string author)const;
      optional<content_object>    get_content_by_url(string url)const;
      vector<content_object> lookup_content(const string& start, uint32_t limit )const;
      vector<content_object> list_content_by_latest( const content_id_type start, uint16_t limit )const;
      vector<content_object> list_content_by_genre( uint32_t genre, const content_id_type start, uint16_t limit )const;
      vector<content_object> list_content_by_category( const string& category, const content_id_type bound, uint16_t limit )const;
      vector<content_object> list_content_by_uploader( const string& uploader, const object_id_type bound, uint16_t limit )const;

      //scoring
      uint64_t get_account_scoring( string account );
      uint64_t get_content_scoring( string content );
      // Market
      order_book get_order_book( uint32_t limit )const;
      order_book get_order_book_for_asset( asset_id_type asset_id, uint32_t limit )const;
      vector< liquidity_balance > get_liquidity_queue( string start_account, uint32_t limit )const;

      //Assets
      vector<asset_object> lookup_uias(uint64_t start_id )const;
      optional<asset_object>         get_uia_details(string UIA)const;
      asset_object get_asset(asset_id_type asset_id)const;
      vector <account_balance_object> get_uia_balances( string account );

      // Authority / validation
      std::string get_transaction_hex(const signed_transaction& trx)const;
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      bool verify_authority( const signed_transaction& trx )const;
      bool verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;

      // signal handlers
      void on_applied_block( const chain::signed_block& b );

      mutable fc::bloom_filter                _subscribe_filter;
      std::function<void(const fc::variant&)> _subscribe_callback;
      std::function<void(const fc::variant&)> _pending_trx_callback;
      std::function<void(const fc::variant&)> _block_applied_callback;

      muse::chain::database&                _db;

      boost::signals2::scoped_connection       _block_applied_connection;

      vector<balance_object> get_balance_objects( const vector<address>& addrs )const;

};



//////////////////////////////////////////////////////////////////////
//                                                                  //
// Subscriptions                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api::set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter )
{
   my->set_subscribe_callback( cb, clear_filter );
}

void database_api_impl::set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter )
{
   _subscribe_callback = cb;
   if( clear_filter || !cb )
   {
      static fc::bloom_parameters param;
      param.projected_element_count    = 10000;
      param.false_positive_probability = 1.0/10000;
      param.maximum_size = 1024*8*8*2;
      param.compute_optimal_parameters();
      _subscribe_filter = fc::bloom_filter(param);
   }
}

void database_api::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   my->set_pending_transaction_callback( cb );
}

void database_api_impl::set_pending_transaction_callback( std::function<void(const variant&)> cb )
{
   _pending_trx_callback = cb;
}

void database_api::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
{
   my->set_block_applied_callback( cb );
}

void database_api_impl::on_applied_block( const chain::signed_block& b )
{
   try
   {
      _block_applied_callback( fc::variant( signed_block_header(b), GRAPHENE_MAX_NESTED_OBJECTS ) );
   }
   catch( ... )
   {
      _block_applied_connection.release();
   }
}

void database_api_impl::set_block_applied_callback( std::function<void(const variant& block_header)> cb )
{
   _block_applied_callback = cb;
   _block_applied_connection = connect_signal( _db.applied_block, *this, &database_api_impl::on_applied_block );
}

void database_api::cancel_all_subscriptions()
{
   my->cancel_all_subscriptions();
}

void database_api_impl::cancel_all_subscriptions()
{
   set_subscribe_callback( std::function<void(const fc::variant&)>(), true);
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api( muse::chain::database& db )
   : my( new database_api_impl( db ) ) {}

database_api::database_api( const muse::app::api_context& ctx )
   : database_api( *ctx.app.chain_database() ) {}

database_api::~database_api() {}

database_api_impl::database_api_impl( muse::chain::database& db ):_db(db)
{
   ilog("creating database api ${x}", ("x",int64_t(this)) );
}

database_api_impl::~database_api_impl()
{
   ilog("freeing database api ${x}", ("x",int64_t(this)) );
}

void database_api::on_api_startup() {}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

optional<block_header> database_api::get_block_header(uint32_t block_num)const
{
   return my->get_block_header( block_num );
}

optional<block_header> database_api_impl::get_block_header(uint32_t block_num) const
{
   auto result = _db.fetch_block_by_number(block_num);
   if(result)
      return *result;
   return {};
}

optional<signed_block> database_api::get_block(uint32_t block_num)const
{
   return my->get_block( block_num );
}

optional<signed_block> database_api_impl::get_block(uint32_t block_num)const
{
   return _db.fetch_block_by_number(block_num);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////=

fc::variant_object database_api::get_config()const
{
   return my->get_config();
}

fc::variant_object database_api_impl::get_config()const
{
   return muse::chain::get_config();
}

dynamic_global_property_object database_api::get_dynamic_global_properties()const
{
   return my->get_dynamic_global_properties();
}

chain_properties database_api::get_chain_properties()const
{
   return my->_db.get_witness_schedule_object().median_props;
}

feed_history_object database_api::get_feed_history()const {
   return my->_db.get_feed_history();
}

price database_api::get_current_median_history_price()const {
   return my->_db.get_feed_history().current_median_history;
}

dynamic_global_property_object database_api_impl::get_dynamic_global_properties()const
{
   return _db.get(dynamic_global_property_id_type());
}

witness_schedule_object database_api::get_witness_schedule()const
{
   return witness_schedule_id_type()( my->_db );
}

hardfork_version database_api::get_hardfork_version()const
{
   return hardfork_property_id_type()( my->_db ).current_hardfork_version;
}

scheduled_hardfork database_api::get_next_scheduled_hardfork() const
{
   scheduled_hardfork shf;
   const auto& hpo = hardfork_property_id_type()( my->_db );
   shf.hf_version = hpo.next_hardfork;
   shf.live_time = hpo.next_hardfork_time;
   return shf;
}


///n//////////////////////////////////////////////////////////////////
//                                                                  //
// Objects                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

fc::variants database_api::get_objects(const vector<object_id_type>& ids)const
{
   return my->get_objects( ids );
}

fc::variants database_api_impl::get_objects(const vector<object_id_type>& ids)const
{
   fc::variants result;
   result.reserve(ids.size());

   std::transform(ids.begin(), ids.end(), std::back_inserter(result),
                  [this](object_id_type id) -> fc::variant {
                     if(auto obj = _db.find_object(id))
                        return obj->to_variant();
                     return {};
                  });

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Keys                                                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<set<string>> database_api::get_key_references( vector<public_key_type> key )const
{
   return my->get_key_references( key );
}

/**
 *  @return all accounts that referr to the key or account id in their owner or active authorities.
 */
vector<set<string>> database_api_impl::get_key_references( vector<public_key_type> keys )const
{
   vector< set<string> > final_result;
   final_result.reserve(keys.size());

   for( auto& key : keys )
   {
      const auto& idx = _db.get_index_type<account_index>();
      const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
      const auto& refs = aidx.get_secondary_index<muse::chain::account_member_index>();
      set<string> result;

      auto itr = refs.account_to_key_memberships.find(key);
      if( itr != refs.account_to_key_memberships.end() )
      {
         for( auto item : itr->second )
            result.insert(item);
      }
      final_result.emplace_back( std::move(result) );
   }

   return final_result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector< extended_account > database_api::get_accounts( const vector< string >& names )const
{
   return my->get_accounts( names );
}

optional < account_object > database_api::get_account_from_id( account_id_type account_id ) const
{
   return my->get_account_from_id(account_id);
}

vector< extended_account > database_api_impl::get_accounts( const vector< string >& names )const
{
   const auto& dgpo = _db.get_dynamic_global_properties();
   const price vesting_price = dgpo.get_vesting_share_price();
   const auto& idx  = _db.get_index_type< account_index >().indices().get< by_name >();
   const auto& vidx = _db.get_index_type< witness_vote_index >().indices().get< by_account_witness >();
   const auto& proposal_idx = _db.get_index_type<proposal_index>();
   const auto& pidx = dynamic_cast<const primary_index<proposal_index>&>(proposal_idx);
   const auto& proposals_by_account = pidx.get_secondary_index<muse::chain::required_approval_index>();

   vector< extended_account > results;
   for( const auto& name: names )
   {
      auto itr = idx.find( name );
      if ( itr == idx.end() ) continue;

      results.push_back( extended_account( *itr ) );
      results.back().muse_power = itr->vesting_shares * vesting_price;

      auto vitr = vidx.lower_bound( boost::make_tuple( itr->get_id(), witness_id_type() ) );
      while( vitr != vidx.end() && vitr->account == itr->get_id() ) {
         results.back().witness_votes.insert(vitr->witness(_db).owner);
         ++vitr;
      }

      const set<proposal_id_type> proposals = proposals_by_account.lookup( name );
      results.back().proposals.reserve( proposals.size() );
      for( const auto proposal_id : proposals )
         results.back().proposals.push_back( proposal_id(_db) );
   }

   return results;
}

optional < account_object > database_api_impl::get_account_from_id( account_id_type account_id ) const
{
   auto  account=_db.find(account_id);
   if (account == nullptr)
      return {};
   return *account;
}

vector<account_id_type> database_api::get_account_references( account_id_type account_id )const
{
   return my->get_account_references( account_id );
}

vector<account_id_type> database_api_impl::get_account_references( account_id_type account_id )const
{
   /*const auto& idx = _db.get_index_type<account_index>();
   const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
   const auto& refs = aidx.get_secondary_index<muse::chain::account_member_index>();
   auto itr = refs.account_to_account_memberships.find(account_id);
   vector<account_id_type> result;

   if( itr != refs.account_to_account_memberships.end() )
   {
      result.reserve( itr->second.size() );
      for( auto item : itr->second ) result.push_back(item);
   }
   return result;*/
   FC_ASSERT( false, "database_api::get_account_references --- Needs to be refactored for muse." );
}

vector <account_balance_object> database_api::get_uia_balances( string account ){
   return my->get_uia_balances(account);
}

vector <account_balance_object> database_api_impl::get_uia_balances( string account ){
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   vector<account_balance_object> result;
   auto itr = accounts_by_name.find(account);
   if ( itr == accounts_by_name.end() )
      return result;
   const auto& balance_by_account = _db.get_index_type<account_balance_index>().indices().get<by_account_asset>();
   auto bitr = balance_by_account.lower_bound ( std::make_tuple(itr->id, MUSE_SYMBOL ) );
   while ( bitr != balance_by_account.end() && bitr->owner == itr->id )
   {
      result.push_back(*bitr);
      ++bitr;
   }
   return result;
}

vector<optional<account_object>> database_api::lookup_account_names(const vector<string>& account_names)const
{
   return my->lookup_account_names( account_names );
}

vector<optional<account_object>> database_api_impl::lookup_account_names(const vector<string>& account_names)const
{
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   vector<optional<account_object> > result;
   result.reserve(account_names.size());
   std::transform(account_names.begin(), account_names.end(), std::back_inserter(result),
                  [&accounts_by_name](const string& name) -> optional<account_object> {
      auto itr = accounts_by_name.find(name);
      return itr == accounts_by_name.end()? optional<account_object>() : *itr;
   });
   return result;
}

set<string> database_api::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   return my->lookup_accounts( lower_bound_name, limit );
}

set<string> database_api_impl::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
{
   //FC_ASSERT( limit <= 1000 );
   const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
   set<string> result;

   for( auto itr = accounts_by_name.lower_bound(lower_bound_name);
        limit-- && itr != accounts_by_name.end();
        ++itr )
   {
      result.insert(itr->name);
   }

   return result;
}

uint64_t database_api::get_account_count()const
{
   return my->get_account_count();
}

uint64_t database_api_impl::get_account_count()const
{
   return _db.get_index_type<account_index>().indices().size();
}

vector< owner_authority_history_object > database_api::get_owner_history( string account )const
{
   vector< owner_authority_history_object > results;

   const auto& hist_idx = my->_db.get_index_type< owner_authority_history_index >().indices().get< by_account >();
   auto itr = hist_idx.lower_bound( account );

   while( itr != hist_idx.end() && itr->account == account )
   {
      results.push_back( *itr );
      ++itr;
   }

   return results;
}

optional< account_recovery_request_object > database_api::get_recovery_request( string account )const
{
   optional< account_recovery_request_object > result;

   const auto& rec_idx = my->_db.get_index_type< account_recovery_request_index >().indices().get< by_account >();
   auto req = rec_idx.find( account );

   if( req != rec_idx.end() )
      result = *req;

   return result;
}
//////////////////////////////////////////////////////////////////////
//                                                                  //
// Proposed transactions                                            //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<proposal_object> database_api::get_proposed_transactions( string id )const
{
   return my->get_proposed_transactions( id );
}

/** TODO: add secondary index that will accelerate this process */
vector<proposal_object> database_api_impl::get_proposed_transactions( string id )const
{
   const auto& idx = _db.get_index_type<proposal_index>();
   vector<proposal_object> result;
   
   idx.inspect_all_objects( [&](const object& obj){
           const proposal_object& p = static_cast<const proposal_object&>(obj);
           //result.push_back(p);
           if( p.required_active_approvals.find( id ) != p.required_active_approvals.end() )
              result.push_back(p);
           else if ( p.required_owner_approvals.find( id ) != p.required_owner_approvals.end() )
              result.push_back(p);
           else if ( p.available_active_approvals.find( id ) != p.available_active_approvals.end() )
              result.push_back(p);
           else if ( p.required_master_content_approvals.find( id ) != p.required_master_content_approvals.end() )
              result.push_back(p);
           else if ( p.required_comp_content_approvals.find( id ) != p.required_comp_content_approvals.end() )
              result.push_back(p);
   });
   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Scoring                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

uint64_t database_api::get_account_scoring( string account )
{
   return my->get_account_scoring(account);
}

uint64_t database_api_impl::get_account_scoring( string account )
{
   FC_ASSERT( account.size() > 0);
   const account_object* ao = nullptr;
   if (std::isdigit(account[0], c_locale))
      ao = _db.find(fc::variant(account).as<account_id_type>(1));
   else
   {
      const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = idx.find(account);
      if (itr != idx.end())
         ao = &*itr;
   }
   if(!ao)
      return 0;
   return _db.get_scoring( *ao );
}

uint64_t database_api::get_content_scoring( string content )
{
   return my->get_content_scoring(content);
}

uint64_t database_api_impl::get_content_scoring( string content )
{
   const auto& idx = _db.get_index_type< content_index >().indices().get< by_url >();
   auto itr = idx.find( content );
   if( itr == idx.end() )
      return 0;
   return _db.get_scoring( *itr );
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses and streaming platforms                                //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<witness_object>> database_api::get_witnesses(const vector<witness_id_type>& witness_ids)const
{
   return my->get_witnesses( witness_ids );
}

vector<optional<witness_object>> database_api_impl::get_witnesses(const vector<witness_id_type>& witness_ids)const
{
   vector<optional<witness_object>> result; result.reserve(witness_ids.size());
   std::transform(witness_ids.begin(), witness_ids.end(), std::back_inserter(result),
                  [this](witness_id_type id) -> optional<witness_object> {
      if(auto o = _db.find(id))
         return *o;
      return {};
   });
   return result;
}

fc::optional<witness_object> database_api::get_witness_by_account( string account_name ) const
{
   return my->get_witness_by_account( account_name );
}

vector< witness_object > database_api::get_witnesses_by_vote( string from, uint32_t limit )const
{
   FC_ASSERT( limit <= 100 );

   vector<witness_object> result;
   result.reserve(limit);

   const auto& name_idx = my->_db.get_index_type< witness_index >().indices().get< by_name >();
   const auto& vote_idx = my->_db.get_index_type< witness_index >().indices().get< by_vote_name >();

   auto itr = vote_idx.begin();
   if( from.size() ) {
      auto nameitr = name_idx.find( from );
      FC_ASSERT( nameitr != name_idx.end(), "invalid witness name ${n}", ("n",from) );
      itr = vote_idx.iterator_to( *nameitr );
   }

   while( itr != vote_idx.end()  &&
          result.size() < limit &&
          itr->votes > 0 )
   {
      result.push_back(*itr);
      ++itr;
   }
   return result;
}

fc::optional<witness_object> database_api_impl::get_witness_by_account( string account_name ) const
{
   const auto& idx = _db.get_index_type< witness_index >().indices().get< by_name >();
   auto itr = idx.find( account_name );
   if( itr != idx.end() )
      return *itr;
   return {};
}

set< string > database_api::lookup_witness_accounts( const string& lower_bound_name, uint32_t limit ) const
{
   return my->lookup_witness_accounts( lower_bound_name, limit );
}

set< string > database_api::lookup_streaming_platform_accounts( const string& lower_bound_name, uint32_t limit ) const
{
   return my->lookup_streaming_platform_accounts( lower_bound_name, limit );
}

bool database_api::is_streaming_platform( string streaming_platform ) const
{
   return my->is_streaming_platform( streaming_platform );
}

set< string > database_api_impl::lookup_witness_accounts( const string& lower_bound_name, uint32_t limit ) const
{
   FC_ASSERT( limit <= 1000 );
   const auto& witnesses_by_id = _db.get_index_type< witness_index >().indices().get< by_id >();

   // get all the names and look them all up, sort them, then figure out what
   // records to return.  This could be optimized, but we expect the
   // number of witnesses to be few and the frequency of calls to be rare
   set< std::string > witnesses_by_account_name;
   for ( const witness_object& witness : witnesses_by_id )
      if ( witness.owner >= lower_bound_name ) // we can ignore anything below lower_bound_name
         witnesses_by_account_name.insert( witness.owner );

   auto end_iter = witnesses_by_account_name.begin();
   while ( end_iter != witnesses_by_account_name.end() && limit-- )
       ++end_iter;
   witnesses_by_account_name.erase( end_iter, witnesses_by_account_name.end() );
   return witnesses_by_account_name;
}

set< string > database_api_impl::lookup_streaming_platform_accounts( const string& lower_bound_name, uint32_t limit ) const
{
   FC_ASSERT( limit <= 1000 );
   const auto& streaming_platforms_by_id = _db.get_index_type< streaming_platform_index >().indices().get< by_id >();

   // get all the names and look them all up, sort them, then figure out what
   // records to return.  This could be optimized, but we expect the
   // number of witnesses to be few and the frequency of calls to be rare
   set< std::string > sp_by_account_name;
   for ( const streaming_platform_object& sp : streaming_platforms_by_id )
      if ( sp.owner >= lower_bound_name ) // we can ignore anything below lower_bound_name
         sp_by_account_name.insert( sp.owner );

   auto end_iter = sp_by_account_name.begin();
   while ( end_iter != sp_by_account_name.end() && limit-- )
       ++end_iter;
   sp_by_account_name.erase( end_iter, sp_by_account_name.end() );
   return sp_by_account_name;
}

bool database_api_impl::is_streaming_platform(string streaming_platform) const
{
   return _db.is_streaming_platform(streaming_platform);
}

uint64_t database_api::get_witness_count()const
{
   return my->get_witness_count();
}

uint64_t database_api_impl::get_witness_count()const
{
   return _db.get_index_type<witness_index>().indices().size();
}

vector<report_object> database_api::get_reports_for_account(string consumer)const
{
   return my->get_reports_for_account(consumer);
}

vector<report_object> database_api_impl::get_reports_for_account(string consumer)const
{
   const auto& idx= _db.get_index_type< report_index >().indices().get< by_consumer>();
   vector <report_object> result;
   account_id_type c=_db.get_account(consumer).id;

   auto itr = idx.lower_bound( c );
   while( itr != idx.end() && itr->consumer == c && result.size() < 1000 )
   {
      result.push_back (*itr);
      ++itr;
   }
   return result;
}

vector<content_object> database_api::get_content_by_uploader(string author)const
{
   return my->get_content_by_uploader(author);
}

vector<content_object> database_api_impl::get_content_by_uploader(string uploader)const
{
   const auto& idx= _db.get_index_type<content_index>().indices().get< by_uploader >();
   vector <content_object> result;
    
   auto itr = idx.lower_bound( std::make_tuple( uploader, content_id_type(-1) ) );
   while( itr != idx.end() && itr->uploader == uploader && result.size() < 1000 )
   {
      result.push_back (*itr);
      ++itr;
   }
   return result;
}

optional<content_object> database_api::get_content_by_url(string url)const
{
   return my->get_content_by_url(url);
}

optional<content_object> database_api_impl::get_content_by_url(string url)const
{
   try{
      optional<content_object> result;
      result = _db.get_content(url);
      return result;
   }
   catch(const fc::exception& e)
   {
      return {};
   }
}


vector<content_object>  database_api::lookup_content(const string& start, uint32_t limit )const
{
   return my->lookup_content(start, limit);
}

vector<content_object>  database_api_impl::lookup_content(const string& start, uint32_t limit )const
{
   vector <content_object> result;
   const auto& idx = _db.get_index_type<content_index>().indices().get<by_title>();
   auto itr = idx.lower_bound( start );
   while( itr!=idx.end() && result.size() < limit && itr->track_title.compare( 0, start.size(), start ) == 0 )
   {
      result.push_back( *itr );
      ++itr;
   }
   return result;
}

vector<content_object> database_api::list_content_by_latest( const string& start, uint16_t limit )const
{
   if( start.empty() )
      return my->list_content_by_latest( content_id_type(), limit );
   return my->list_content_by_latest( fc::variant(start, 1).as<content_id_type>(1), limit );
}

vector<content_object> database_api_impl::list_content_by_latest( const content_id_type start, uint16_t limit )const
{
   FC_ASSERT( limit <= 100 );

   vector<content_object> result;
   result.reserve( limit );
   const auto& idx = _db.get_index_type<content_index>().indices().get<by_id>();
   auto itr = (start.instance.value > 0 ? idx.upper_bound( start ) : idx.end());
   if( itr == idx.begin() ) return result;
   if( start.instance.value > 0 )
   {
      --itr;
      if( itr->id != start ) itr++;
   }
   while( itr != idx.begin() && result.size() < limit )
      result.push_back( *--itr );

   return result;
}

vector<content_object> database_api::list_content_by_genre( uint32_t genre, const string& bound, uint16_t limit )const
{
   if( bound.empty() )
      return my->list_content_by_genre( genre, content_id_type(), limit );
   return my->list_content_by_genre( genre, fc::variant(bound, 1).as<content_id_type>(1), limit );
}

vector<content_object> database_api_impl::list_content_by_genre( uint32_t genre, const content_id_type bound, uint16_t limit )const
{
   FC_ASSERT( limit <= 100 );

   vector<content_object> result;
   result.reserve( limit );
   const auto& idx = _db.get_index_type< primary_index< content_index > >();
   const content_by_genre_index& by_genre = idx.get_secondary_index<muse::chain::content_by_genre_index>();
   const set< content_id_type > ids = by_genre.find_by_genre( genre );
   auto itr = (bound.instance.value > 0 ? ids.upper_bound( bound ) : ids.end());
   if( itr == ids.begin() ) return result;
   if( bound.instance.value > 0 )
   {
      --itr;
      if( *itr != bound ) itr++;
   }
   while( itr != ids.begin() && result.size() < limit )
      result.push_back( (*--itr)(_db) );

   return result;
}

vector<content_object> database_api::list_content_by_category( const string& category, const string& bound, uint16_t limit )const
{
   if( bound.empty() )
      return my->list_content_by_category( category, content_id_type(), limit );
   return my->list_content_by_category( category, fc::variant(bound, 1).as<content_id_type>(1), limit );
}

vector<content_object> database_api_impl::list_content_by_category( const string& category, const content_id_type bound, uint16_t limit )const
{
   FC_ASSERT( limit <= 100 );

   vector<content_object> result;
   result.reserve( limit );
   const auto& idx = _db.get_index_type< primary_index< content_index > >();
   const content_by_category_index& by_category = idx.get_secondary_index<muse::chain::content_by_category_index>();
   const set< content_id_type > ids = by_category.find_by_category( category );
   auto itr = (bound.instance.value > 0 ? ids.upper_bound( bound ) : ids.end());
   if( itr == ids.begin() ) return result;
   if( bound.instance.value > 0 )
   {
      --itr;
      if( *itr != bound ) itr++;
   }
   while( itr != ids.begin() && result.size() < limit )
      result.push_back( (*--itr)(_db) );

   return result;
}

vector<content_object> database_api::list_content_by_uploader( const string& uploader, const string& bound, uint16_t limit )const
{
   if( bound.empty() )
      return my->list_content_by_uploader( uploader, content_id_type(), limit );
   return my->list_content_by_uploader( uploader, fc::variant(bound, 1).as<content_id_type>(1), limit );
}

vector<content_object> database_api_impl::list_content_by_uploader( const string& uploader, const object_id_type bound, uint16_t limit )const
{
   FC_ASSERT( limit <= 100 );

   vector<content_object> result;
   result.reserve( limit );
   const auto& idx = _db.get_index_type<content_index>().indices().get<by_uploader>();
   auto itr = idx.lower_bound( boost::make_tuple( uploader, bound.instance() > 0 ? bound : object_id_type(content_id_type((1ULL<<48)-1)) ) );
   if( itr == idx.end() ) return result;
   if( bound.instance() > 0 )
   {
      if( itr->id == bound ) itr++;
   }
   while( itr != idx.end() && itr->uploader == uploader && result.size() < limit )
      result.push_back( *itr++ );

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Market                                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

order_book database_api::get_order_book( uint32_t limit )const
{
   return my->get_order_book( limit );
}

vector<extended_limit_order> database_api::get_open_orders( string owner )const {
   vector<extended_limit_order> result;
   const auto& idx = my->_db.get_index_type<limit_order_index>().indices().get<by_account>();
   auto itr = idx.lower_bound( owner );
   while( itr != idx.end() && itr->seller == owner ) {
      result.push_back( extended_limit_order( *itr ) );

      if( itr->sell_price.base.asset_id == MUSE_SYMBOL )
         result.back().real_price = (result.back().sell_price).to_real();
      else
         result.back().real_price = (~result.back().sell_price).to_real();
      ++itr;
   }
   return result;
}

order_book database_api_impl::get_order_book( uint32_t limit )const
{ //TODO_MUSE - change to support more symbols
   FC_ASSERT( limit <= 1000 );
   order_book result;

   auto max_sell = price::max( MBD_SYMBOL, MUSE_SYMBOL );
   auto max_buy = price::max( MUSE_SYMBOL, MBD_SYMBOL );

   const auto& limit_price_idx = _db.get_index_type<limit_order_index>().indices().get<by_price>();
   auto sell_itr = limit_price_idx.lower_bound(max_sell);
   auto buy_itr  = limit_price_idx.lower_bound(max_buy);
   auto end = limit_price_idx.end();

   while(  sell_itr != end && sell_itr->sell_price.base.asset_id == MBD_SYMBOL && result.bids.size() < limit )
   {
      auto itr = sell_itr;
      order cur;
      cur.order_price = itr->sell_price;
      cur.real_price  = (cur.order_price).to_real();
      cur.quote = itr->for_sale;
      cur.base = ( asset( itr->for_sale, MBD_SYMBOL ) * cur.order_price ).amount;
      cur.created = itr->created;
      result.bids.push_back( cur );
      ++sell_itr;
   }
   while(  buy_itr != end && buy_itr->sell_price.base.asset_id == MUSE_SYMBOL && result.asks.size() < limit )
   {
      auto itr = buy_itr;
      order cur;
      cur.order_price = itr->sell_price;
      cur.real_price  = (~cur.order_price).to_real();
      cur.base   = itr->for_sale;
      cur.quote     = ( asset( itr->for_sale, MUSE_SYMBOL ) * cur.order_price ).amount;
      cur.created = itr->created;
      result.asks.push_back( cur );
      ++buy_itr;
   }

   return result;
}

order_book database_api::get_order_book_for_asset( asset_id_type asset_id, uint32_t limit )const
{
   return my->get_order_book_for_asset( asset_id, limit ); 
}
order_book database_api_impl::get_order_book_for_asset( asset_id_type asset_id, uint32_t limit )const
{ 
   FC_ASSERT( limit <= 1000 );
   order_book result;
   result.base = MUSE_SYMBOL;
   result.quote = asset_id;

   const auto& limit_price_idx = _db.get_index_type<limit_order_index>().indices().get<by_price>();
   auto sell_itr = limit_price_idx.lower_bound( price::max( MUSE_SYMBOL, asset_id ) );
   auto sell_end  = limit_price_idx.upper_bound(  price::min( MUSE_SYMBOL, asset_id ) );
   auto buy_itr = limit_price_idx.lower_bound( price::max( asset_id, MUSE_SYMBOL ) );
   auto buy_end  = limit_price_idx.upper_bound(  price::min( asset_id, MUSE_SYMBOL ) );
   
   uint32_t count = 0;
   while(  sell_itr != sell_end && count < limit )
   {
      auto itr = sell_itr;
      order cur;
      cur.order_price = itr->sell_price;
      cur.real_price  = (cur.order_price).to_real();
      cur.quote = itr->for_sale;
      cur.base = ( asset( itr->for_sale, asset_id ) * cur.order_price ).amount;
      cur.created = itr->created;
      result.bids.push_back( cur );
      ++sell_itr;
      ++count;
   }
   count = 0;
   while(  buy_itr != buy_end && count < limit )
   {
      auto itr = buy_itr;
      order cur;
      cur.order_price = itr->sell_price;
      cur.real_price  = (~cur.order_price).to_real();
      cur.base  = itr->for_sale;
      cur.quote  = ( asset( itr->for_sale, MUSE_SYMBOL ) * cur.order_price ).amount;
      cur.created = itr->created;
      result.asks.push_back( cur );
      ++buy_itr;
      ++count;
   }
   return result;
}

vector< liquidity_balance > database_api::get_liquidity_queue( string start_account, uint32_t limit )const
{
   return my->get_liquidity_queue( start_account, limit );
}

vector< liquidity_balance > database_api_impl::get_liquidity_queue( string start_account, uint32_t limit )const
{
   FC_ASSERT( limit <= 1000 );

   const auto& liq_idx = _db.get_index_type< liquidity_reward_index >().indices().get< by_volume_weight >();
   auto itr = liq_idx.begin();
   vector< liquidity_balance > result;

   result.reserve( limit );

   if( start_account.length() )
   {
      const auto& liq_by_acc = _db.get_index_type< liquidity_reward_index >().indices().get< by_owner >();
      auto acc = liq_by_acc.find( _db.get_account( start_account ).id );

      if( acc != liq_by_acc.end() )
      {
         itr = liq_idx.find( boost::make_tuple( acc->weight, acc->owner ) );
      }
      else
      {
         itr = liq_idx.end();
      }
   }

   while( itr != liq_idx.end() && result.size() < limit )
   {
      liquidity_balance bal;
      bal.account = itr->owner( _db ).name;
      bal.weight = itr->weight;
      result.push_back( bal );

      ++itr;
   }

   return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// UIA                                                              //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<asset_object> database_api::lookup_uias(uint64_t start_id)const
{
   return my->lookup_uias(start_id);
}

optional<asset_object> database_api::get_uia_details(string UIA)const
{
   return my->get_uia_details(UIA);
}

asset_object database_api::get_asset(asset_id_type asset_id)const
{
   return my->get_asset(asset_id);
}

vector<asset_object> database_api_impl::lookup_uias(uint64_t start_id )const
{
   vector<asset_object> result;
   const auto& idx=_db.get_index_type<asset_index>().indices().get<by_id>();
   auto itr=idx.find( asset_id_type(start_id) );
   while ( itr != idx.end() )
   {
      result.push_back(*itr);
      itr++;
   }
   return result;
}

optional<asset_object> database_api_impl::get_uia_details(string UIA)const
{
   const auto& idx=_db.get_index_type<asset_index>().indices().get<by_symbol>();
   auto itr=idx.find(UIA);
   if(itr != idx.end())
      return *itr;
   else
      return {};
}

asset_object database_api_impl::get_asset(asset_id_type asset_id)const
{
   return _db.get(asset_id);
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::string database_api::get_transaction_hex(const signed_transaction& trx)const
{
   return my->get_transaction_hex( trx );
}

std::string database_api_impl::get_transaction_hex(const signed_transaction& trx)const
{
   return fc::to_hex(fc::raw::pack_to_vector(trx));
}

set<public_key_type> database_api::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   return my->get_required_signatures( trx, available_keys );
}

set<public_key_type> database_api_impl::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
{
   auto result = trx.get_required_signatures( MUSE_CHAIN_ID,
                                              available_keys,
                                              [&]( string account_name ){ return &_db.get_account( account_name ).active; },
                                              [&]( string account_name ){ return &_db.get_account( account_name ).owner; },
                                              [&]( string account_name ){ return &_db.get_account( account_name ).basic; },
                                              [&]( string content_url ){ return &_db.get_content( content_url ).manage_master; },
                                              [&]( string content_url ){ return &_db.get_content( content_url ).manage_comp; },
                                              MUSE_MAX_SIG_CHECK_DEPTH );
   return result;
}

set<public_key_type> database_api::get_potential_signatures( const signed_transaction& trx )const
{
   return my->get_potential_signatures( trx );
}

set<public_key_type> database_api_impl::get_potential_signatures( const signed_transaction& trx )const
{
   set<public_key_type> result;
   trx.get_required_signatures(
      MUSE_CHAIN_ID,
      flat_set<public_key_type>(),
      [&]( string account_name )
      {
         const auto& auth = _db.get_account(account_name).active;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      [&]( string account_name )
      {
         const auto& auth = _db.get_account(account_name).owner;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      [&]( string account_name )
      {
         const auto& auth = _db.get_account(account_name).basic;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      [&]( string url )
      {
         const auto& auth = _db.get_content( url ).manage_master;
         for( const auto& k : auth.get_keys() )
            result.insert(k);
         return &auth;
      },
      [&]( string url )
      {
           const auto& auth = _db.get_content( url ).manage_comp;
           for( const auto& k : auth.get_keys() )
              result.insert(k);
           return &auth;
      },
      MUSE_MAX_SIG_CHECK_DEPTH
   );

   return result;
}

bool database_api::verify_authority( const signed_transaction& trx ) const
{
   return my->verify_authority( trx );
}

bool database_api_impl::verify_authority( const signed_transaction& trx )const
{
   trx.verify_authority( MUSE_CHAIN_ID,
                         [&]( string account_name ){ return &_db.get_account( account_name ).active; },
                         [&]( string account_name ){ return &_db.get_account( account_name ).owner; },
                         [&]( string account_name ){ return &_db.get_account( account_name ).basic; },
                         [&]( string content_url ){ return &_db.get_content( content_url ).manage_master; },
                         [&]( string content_url ){ return &_db.get_content( content_url ).manage_comp; },
                         !_db.has_hardfork( MUSE_HARDFORK_0_3 ) ? 1 : 2 );
   return true;
}

bool database_api::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const
{
   return my->verify_account_authority( name_or_id, signers );
}

bool database_api_impl::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& keys )const
{
   FC_ASSERT( name_or_id.size() > 0);
   const account_object* account = nullptr;
   if (std::isdigit(name_or_id[0], c_locale))
      account = _db.find(fc::variant(name_or_id, 1).as<account_id_type>(1));
   else
   {
      const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = idx.find(name_or_id);
      if (itr != idx.end())
         account = &*itr;
   }
   FC_ASSERT( account, "no such account" );


   /// reuse trx.verify_authority by creating a dummy transfer
   signed_transaction trx;
   transfer_operation op;
   op.from = account->id;
   trx.operations.emplace_back(op);

   return verify_authority( trx );
}

vector<convert_request_object> database_api::get_conversion_requests( const string& account )const {
  const auto& idx = my->_db.get_index_type<convert_index>().indices().get<by_owner>();
  vector<convert_request_object> result;
  auto itr = idx.lower_bound(account);
  while( itr != idx.end() && itr->owner == account ) {
     result.push_back(*itr);
     ++itr;
  }
  return result;
}



u256 to256( const fc::uint128& t ) {
   u256 result( t.high_bits() );
   result <<= 65;
   result += t.low_bits();
   return result;
}



map<uint32_t,operation_object> database_api::get_account_history( string account, uint64_t from, uint32_t limit )const {
   FC_ASSERT( limit <= 2000, "Limit of ${l} is greater than maxmimum allowed", ("l",limit) );
   FC_ASSERT( from >= limit, "From must be greater than limit" );
   const auto& idx = my->_db.get_index_type<account_history_index>().indices().get<by_account>();
   auto itr = idx.lower_bound( boost::make_tuple( account, from ) );
   auto end = idx.upper_bound( boost::make_tuple( account, std::max( int64_t(0), int64_t(itr->sequence)-limit ) ) );

   map<uint32_t,operation_object> result;
   while( itr != end ) {
      result[itr->sequence] = itr->op(my->_db);
      ++itr;
   }
   return result;
}



vector<string> database_api::get_active_witnesses()const {
   const auto& wso = my->_db.get_witness_schedule_object();
   return wso.current_shuffled_witnesses;
}

vector<string> database_api::get_voted_streaming_platforms()const {
   return my->_db.get_voted_streaming_platforms();
}


/*state database_api::get_state( string path )const
{
   state _state;
   _state.props         = get_dynamic_global_properties();
   _state.current_route = path;
   _state.feed_price    = get_current_median_history_price();

   try {
   if( path.size() && path[0] == '/' )
      path = path.substr(1); /// remove '/' from front

   if( !path.size() )
      path = "trending";

   set<string> accounts;

   vector<string> part; part.reserve(4);
   boost::split( part, path, boost::is_any_of("/") );
   part.resize(std::max( part.size(), size_t(4) ) ); // at least 4

   auto tag = fc::to_lower( part[1] );

   if( part[0].size() && part[0][0] == '@' ) {
      auto acnt = part[0].substr(1);
      _state.accounts[acnt] = my->_db.get_account(acnt);
      auto& eacnt = _state.accounts[acnt];
      if( part[1] == "transfers" ) {
         auto history = get_account_history( acnt, uint64_t(-1), 1000 );
         for( auto& item : history ) {
            switch( item.second.op.which() ) {
               case operation::tag<transfer_to_vesting_operation>::value:
               case operation::tag<withdraw_vesting_operation>::value:
               case operation::tag<interest_operation>::value:
               case operation::tag<transfer_operation>::value:
               case operation::tag<liquidity_reward_operation>::value:
               case operation::tag<curate_reward_operation>::value:
                  eacnt.transfer_history[item.first] =  item.second;
                  break;
               case operation::tag<comment_operation>::value:
               //   eacnt.post_history[item.first] =  item.second;
                  break;
               case operation::tag<limit_order_create_operation>::value:
               case operation::tag<limit_order_cancel_operation>::value:
               case operation::tag<fill_convert_request_operation>::value:
               case operation::tag<fill_order_operation>::value:
               //   eacnt.market_history[item.first] =  item.second;
                  break;
               case operation::tag<vote_operation>::value:
               case operation::tag<account_witness_vote_operation>::value:
               case operation::tag<account_witness_proxy_operation>::value:
               //   eacnt.vote_history[item.first] =  item.second;
                  break;
               case operation::tag<account_create_operation>::value:
               case operation::tag<account_update_operation>::value:
               case operation::tag<witness_update_operation>::value:
               case operation::tag<pow_operation>::value:
               case operation::tag<custom_operation>::value:
               default:
                  eacnt.other_history[item.first] =  item.second;
            }
         }
      } else if( part[1] == "recent-replies" ) {
        auto replies = get_replies_by_last_update( acnt, "", 50 );
        eacnt.recent_replies = vector<string>();
        for( const auto& reply : replies ) {
           auto reply_ref = reply.author+"/"+reply.permlink;
           _state.content[ reply_ref ] = reply;
           eacnt.recent_replies->push_back( reply_ref );
        }
      } else if( part[1] == "posts" ) {
        int count = 0;
        const auto& pidx = my->_db.get_index_type<comment_index>().indices().get<by_author_last_update>();
        auto itr = pidx.lower_bound( boost::make_tuple(acnt, time_point_sec::maximum() ) );
        eacnt.posts = vector<string>();
        while( itr != pidx.end() && itr->author == acnt && count < 20 ) {
           eacnt.posts->push_back(itr->permlink);
           _state.content[acnt+"/"+itr->permlink] = *itr;
           set_pending_payout( _state.content[acnt+"/"+itr->permlink] );
           ++itr;
           ++count;
        }
      } else if( part[1].size() == 0 || part[1] == "blog" ) {
           int count = 0;
           const auto& pidx = my->_db.get_index_type<comment_index>().indices().get<by_blog>();
           auto itr = pidx.lower_bound( boost::make_tuple(acnt, std::string(""), time_point_sec::maximum() ) );
           eacnt.blog = vector<string>();
           while( itr != pidx.end() && itr->author == acnt && count < 20 && !itr->parent_author.size() ) {
              eacnt.blog->push_back(itr->permlink);
              _state.content[acnt+"/"+itr->permlink] = *itr;
              set_pending_payout( _state.content[acnt+"/"+itr->permlink] );
              ++itr;
              ++count;
           }
      } else if( part[1].size() == 0 || part[1] == "feed" ) {
         const auto& fidxs = my->_db.get_index_type<follow::feed_index>().indices();
         const auto& fidx = fidxs.get<muse::follow::by_account>();

         auto itr = fidx.lower_bound( eacnt.id );
         int count = 0;
         while( itr != fidx.end() && itr->account == eacnt.id && count < 100 ) {
            const auto& c = itr->comment( my->_db );
            const auto link = c.author + "/" + c.permlink;
            _state.content[link] = c;
            eacnt.feed->push_back( link );
            set_pending_payout( _state.content[link] );
            ++itr;
            ++count;
         }
      }
   }
   /// pull a complete discussion
   else if( part[1].size() && part[1][0] == '@' ) {

      auto account  = part[1].substr( 1 );
      auto category = part[0];
      auto slug     = part[2];

      auto key = account +"/" + slug;
      auto dis = get_content( account, slug );
      recursively_fetch_content( _state, dis, accounts );
   }
   else if( part[0] == "witnesses" || part[0] == "~witnesses") {
      auto wits = get_witnesses_by_vote( "", 50 );
      for( const auto& w : wits ) {
         _state.witnesses[w.owner] = w;
      }
      _state.pow_queue = get_miner_queue();
   }
   else {
      elog( "What... no matches" );
   }

   for( const auto& a : accounts )
   {
      _state.accounts.erase("");
      _state.accounts[a] = my->_db.get_account( a );
   }


   _state.witness_schedule = my->_db.get_witness_schedule_object();

 } catch ( const fc::exception& e ) {
    _state.error = e.to_detail_string();
 }
 return _state;
}*/

annotated_signed_transaction database_api::get_transaction( transaction_id_type id )const {
   const auto& idx = my->_db.get_index_type<operation_index>().indices().get<by_transaction_id>();
   auto itr = idx.lower_bound( id );
   if( itr != idx.end() && itr->trx_id == id ) {
      auto blk = my->_db.fetch_block_by_number( itr->block );
      FC_ASSERT( blk.valid() );
      FC_ASSERT( blk->transactions.size() > itr->trx_in_block );
      annotated_signed_transaction result = blk->transactions[itr->trx_in_block];
      result.block_num       = itr->block;
      result.transaction_num = itr->trx_in_block;
      return result;
   }
   FC_ASSERT( false, "Unknown Transaction ${t}", ("t",id));
}


vector<balance_object> database_api::get_balance_objects( const vector<address>& addrs )const
{
   return my->get_balance_objects( addrs );
}

vector<balance_object> database_api::get_balance_objects_by_key( const string& pubkey )const
{
   vector< address > addrs;
   addrs.reserve( 5 );

   fc::ecc::public_key pk = fc::ecc::public_key::from_base58(pubkey);
   addrs.push_back( address(pk) );
   addrs.push_back( pts_address( pk, false, 56 ) );
   addrs.push_back( pts_address( pk, true, 56 ) );
   addrs.push_back( pts_address( pk, false, 0 ) );
   addrs.push_back( pts_address( pk, true, 0 ) );
   return my->get_balance_objects(addrs);
}

vector<balance_object> database_api_impl::get_balance_objects( const vector<address>& addrs )const
{
   try
   {
      const auto& bal_idx = _db.get_index_type<balance_index>();
      const auto& by_owner_idx = bal_idx.indices().get<by_owner>();

      vector<balance_object> result;

      for( const auto& owner : addrs )
      {
         auto itr = by_owner_idx.lower_bound(  owner  );
         while( itr != by_owner_idx.end() && itr->owner == owner )
         {
            result.push_back( *itr );
            ++itr;
         }
      }
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (addrs) )
}


} } // muse::app
