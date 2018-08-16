#pragma once
#include <muse/app/state.hpp>
#include <muse/chain/protocol/types.hpp>

#include <muse/chain/database.hpp>
#include <muse/chain/base_objects.hpp>
#include <muse/chain/history_object.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace muse { namespace app {

using namespace muse::chain;
using namespace std;

struct order
{
   price                order_price;
   double               real_price; // dollars per muse
   share_type           base;
   share_type           quote;
   fc::time_point_sec   created;
};

struct order_book
{
   string               base = MUSE_SYMBOL_STRING;
   string               quote = MBD_SYMBOL_STRING;
   vector< order >      asks;
   vector< order >      bids;
};

struct api_context;

struct scheduled_hardfork
{
   hardfork_version     hf_version;
   fc::time_point_sec   live_time;
};

struct liquidity_balance
{
   string               account;
   fc::uint128_t        weight;
};

class database_api_impl;

/**
 *  Defines the arguments to a query as a struct so it can be easily extended
 */
struct discussion_query {
   void validate()const{
      FC_ASSERT( filter_tags.find(tag) == filter_tags.end() );
      FC_ASSERT( limit <= 100 );
   }

   string           tag;
   uint32_t         limit;
   set<string>      filter_tags;
   optional<string> start_author;
   optional<string> start_permlink;
   optional<string> parent_author;
   optional<string> parent_permlink;
};

/**
 * @brief The database_api class implements the RPC API for the chain database.
 *
 * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
 * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
 * the @ref network_broadcast_api.
 */
class database_api
{
   public:
      database_api(muse::chain::database& db);
      database_api(const muse::app::api_context& ctx);
      ~database_api();

      /*****************
       * Get reports streaming related to given consumer
       * @param consumer Account whose report we retrieve
       * @return List of reports, not more than 1000 most recent entries
       * @ingroup db_api
       ****************/
      vector<report_object> get_reports_for_account(string consumer)const;
      
      // vector<something> get_reports_for_content()
      
      /***************
       * Get list of content uploaded by given account
       * @param author Account uploading the content
       * @return List of content, not more than 1000 entries
       * @ingroup db_api
       */
      vector<content_object> get_content_by_uploader(string author)const;
      
      /****************
       * Get piece of content by its url
       * @param url URL to retrieve
       * @return Content object if content with given url has been found, empty otherwise
       * @ingroup db_api
       */
      optional<content_object>    get_content_by_url(string url)const;

      /****************
       * Lookup songs by title
       * @param start First letters of the title to look for
       * @param limit Lenght of the list to retrieve
       * @return List of content, sorted by title
       * @ingroup db_api
       */
      vector<content_object>  lookup_content(const string& start, uint32_t limit )const;

      /****************
       * Lookup songs by descending publication (in MUSE!) time
       * @param bound if not empty and not 2.9.0, list only content_objects *smaller than* that content_id
       * @param limit Length of the list to retrieve (max 100)
       * @return List of content, sorted by descending publication time
       * @ingroup db_api
       */
      vector<content_object> list_content_by_latest( const string& bound, uint16_t limit )const;

      /****************
       * Lookup songs matching the given genre by descending publication (in MUSE!) time
       * @param genre the genre id
       * @param bound if not empty and not 2.9.0, list only content_objects *smaller than* that content_id
       * @param limit Length of the list to retrieve (max 100)
       * @return List of content, sorted by descending publication time
       * @ingroup db_api
       */
      vector<content_object> list_content_by_genre( uint32_t genre, const string& bound, uint16_t limit )const;

      /****************
       * Lookup songs matching the given category (aka album_meta.album_type) by
       * descending publication (in MUSE!) time
       * @param category the category name
       * @param bound if not empty and not 2.9.0, list only content_objects *smaller than* that content_id
       * @param limit Length of the list to retrieve (max 100)
       * @return List of content, sorted by descending publication time
       * @ingroup db_api
       */
      vector<content_object> list_content_by_category( const string& category, const string& bound, uint16_t limit )const;

      /****************
       * Lookup songs that were uploaded by the given account by descending
       * publication (in MUSE!) time
       * @param uploader the uploader account name
       * @param bound if not empty and not 2.9.0, list only content_objects *smaller than* that content_id
       * @param limit Length of the list to retrieve (max 100)
       * @return List of content, sorted by descending publication time
       * @ingroup db_api
       */
      vector<content_object> list_content_by_uploader( const string& uploader, const string& bound, uint16_t limit )const;

      /****************
       * Lookup User Issued Assets
       * @param start_id the ID to start with
       * @return List of UIA asset objects
       * @ingroup db_api
       */
      vector<asset_object> lookup_uias( uint64_t start_id )const;

      /****************
       * Get details about particular User Issued Assets
       * @param UIA Name of the asset to look for
       * @return Asset object if found, empty otherwise
       * @ingroup db_api
       */
      optional<asset_object> get_uia_details( string UIA )const;
      
      /****************
       * Get details about particular User Issued Assets
       * @param asset_id ID of the asset to look for
       * @return Asset object
       * @ingroup db_api
       */
      asset_object get_asset( asset_id_type asset_id )const;
      
      /****************
       * Get User Issued Asset balances for a particular account
       * @param account Account to look for
       * @return List of balances
       * @ingroup db_api
       */
      vector <account_balance_object> get_uia_balances( string account );

      /**
       * Get score of the given account
       * @param account Account to look for
       * @return Score
       * @ingroup db_api
       */

      uint64_t get_account_scoring( string account );
      /**
       * Get score of the given content
       * @param content Content to look for
       * @return Score
       * @ingroup db_api
       */

      uint64_t get_content_scoring( string content );

      ///////////////////
      // Subscriptions //
      ///////////////////

      void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      /**
       * @brief Set callback to be called after new block has been applied
       * @cb Callback
       * @ingroup db_api
       */
      void set_block_applied_callback( std::function<void(const variant& block_header)> cb );
      
      /**
       * @brief Stop receiving any notifications
       *
       * This unsubscribes from all subscribed markets and objects.
       * @ingroup db_api
       */
      void cancel_all_subscriptions();

      /**
       *  This API is a short-cut for returning all of the state required for a particular URL
       *  with a single query. Not used in MUSE
       */
      //state get_state( string path )const;
      
      /*********************
       * Get list of active witnesses
       * @return List of witnesses
       * @ingroup db_api
       */
      vector<string> get_active_witnesses()const;
      
      /*********************
       * Get list of active streaming platforms
       * @return List of streaming platforms
       * @ingroup db_api
       */
      vector<string> get_voted_streaming_platforms()const;
      
      /*********************
       * Checks if given streaming platform exists and is voted in
       * @param streaming_platform Name of the owner's account
       * @return True if the given owner has an active streaming platform, false otherwise
       * @ingroup db_api
       */
      bool           is_streaming_platform(string owner)const;

      /////////////////////////////
      // Blocks and transactions //
      /////////////////////////////

      /**
       * @brief Retrieve a block header
       * @param block_num Height of the block whose header should be returned
       * @return header of the referenced block, or null if no matching block was found
       * @ingroup db_api
       */
      optional<block_header> get_block_header(uint32_t block_num)const;

      /**
       * @brief Retrieve a full, signed block
       * @param block_num Height of the block to be returned
       * @return the referenced block, or null if no matching block was found
       * @ingroup db_api
       */
      optional<signed_block> get_block(uint32_t block_num)const;
      /**
       *  @return the set of proposed transactions relevant to the specified account id.
       *  @ingroup db_api
       */
      vector<proposal_object> get_proposed_transactions( string id )const;
            
      /////////////
      // Globals //
      /////////////

      /**
       * @brief Retrieve compile-time constants
       * @ingroup db_api
       */
      fc::variant_object get_config()const;

      /**
       * @brief Get the objects corresponding to the provided IDs
       * @param ids IDs of the objects to retrieve
       * @return The objects retrieved, in the order they are mentioned in ids
       *
       * If any of the provided IDs does not map to an object, a null variant is returned in its position.
       * @ingroup db_api
       */
      fc::variants get_objects(const vector<object_id_type>& ids)const;

      /**
       * @brief Retrieve the current @ref dynamic_global_property_object
       * @ingroup db_api
       */
      dynamic_global_property_object get_dynamic_global_properties()const;
      chain_properties               get_chain_properties()const;
      price                          get_current_median_history_price()const;
      feed_history_object            get_feed_history()const;
      witness_schedule_object        get_witness_schedule()const;
      hardfork_version               get_hardfork_version()const;
      scheduled_hardfork             get_next_scheduled_hardfork()const;

      //////////
      // Keys //
      //////////

      vector<set<string>> get_key_references( vector<public_key_type> key )const;

      //////////////
      // Accounts //
      //////////////

      /************
       * Get list of accounts with given names
       * @param names List of names to look for
       * @return List of account objects
       */
      vector< extended_account > get_accounts( const vector< string >& names ) const;
      /***********
       * Get account information from its ID
       * @param account_id ID to look for
       * @return Account object if found, empty otherwise
       */
      optional<account_object> get_account_from_id(account_id_type account_id) const;

      /**
       *  @return all accounts that referr to the key or account id in their owner or active authorities.
       */
      vector<account_id_type> get_account_references( account_id_type account_id )const;

      /**
       * @brief Get a list of accounts by name
       * @param account_names Names of the accounts to retrieve
       * @return The accounts holding the provided names
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;

      /**
       * @brief Get names and IDs for registered accounts
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of account names to corresponding IDs
       */
      set<string> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;

      /**
       * @brief Get the total number of accounts registered with the blockchain
       */
      uint64_t get_account_count()const;

      vector< owner_authority_history_object > get_owner_history( string account )const;

      optional< account_recovery_request_object > get_recovery_request( string account ) const;

      ///////////////
      // Witnesses //
      ///////////////

      /**
       * @brief Get a list of witnesses by ID
       * @param witness_ids IDs of the witnesses to retrieve
       * @return The witnesses corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<witness_object>> get_witnesses(const vector<witness_id_type>& witness_ids)const;

      vector<convert_request_object> get_conversion_requests( const string& account_name )const;

      /**
       * @brief Get the witness owned by a given account
       * @param account The name of the account whose witness should be retrieved
       * @return The witness object, or null if the account does not have a witness
       */
      fc::optional< witness_object > get_witness_by_account( string account_name )const;

      /**
       *  This method is used to fetch witnesses with pagination.
       *
       *  @return an array of `count` witnesses sorted by total votes after witness `from` with at most `limit' results.
       */
      vector< witness_object > get_witnesses_by_vote( string from, uint32_t limit )const;

     /**
       * @brief Get names and IDs for registered witnesses
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of witness names to corresponding IDs
       */
      set<string> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit)const;

     /**
       * @brief Get names and IDs for registered streaming_platforms
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Map of streaming platform names to corresponding IDs
       */
      set<string> lookup_streaming_platform_accounts(const string& lower_bound_name, uint32_t limit)const;
 
      /**
       * @brief Get the total number of witnesses registered with the blockchain
       */
      uint64_t get_witness_count()const;

      ////////////
      // Market //
      ////////////

      /**
       * @breif Gets the current order book for ASSET:SBD market
       * @param limit Maximum number of orders for each side of the spread to return -- Must not exceed 1000
       * @return Order book
       */
      order_book get_order_book( uint32_t limit = 1000 )const;
      /**
       * Gets the current order book for the given asset market
       * @param asset_id Asset ID to look for
       * @param limit Maximum number of orders for each side of the spread to return -- Must not exceed 1000
       * @return Order book
       */
      order_book get_order_book_for_asset( asset_id_type asset_id,  uint32_t limit = 1000 )const;
      /**
       * Get open orders by the given account
       * @param owner Account opening the orders
       * @return List of orders
       */
      vector<extended_limit_order> get_open_orders( string owner )const;

      /**
       * @breif Gets the current liquidity reward queue.
       * @param start_account The account to start the list from, or "" to get the head of the queue
       * @param limit Maxmimum number of accounts to return -- Must not exceed 1000
       */
      vector< liquidity_balance > get_liquidity_queue( string start_account, uint32_t limit = 1000 )const;

      ////////////////////////////
      // Authority / validation //
      ////////////////////////////

      /// @brief Get a hexdump of the serialized binary form of a transaction
      std::string                   get_transaction_hex(const signed_transaction& trx)const;
      annotated_signed_transaction  get_transaction( transaction_id_type trx_id )const;

      /**
       *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
       *  and return the minimal subset of public keys that should add signatures to the transaction.
       */
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;

      /**
       *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
       *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
       *  to get the minimum subset.
       */
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;

      /**
       * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
       */
      bool           verify_authority( const signed_transaction& trx )const;

      /*
       * @return true if the signers have enough authority to authorize an account
       */
      bool           verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;


   ///@}

      /**
       *  For each of these filters:
       *     Get root content...
       *     Get any content...
       *     Get root content in category..
       *     Get any content in category...
       *
       *  Return discussions
       *     Total Discussion Pending Payout
       *     Last Discussion Update (or reply)... think
       *     Top Discussions by Total Payout
       *
       *  Return content (comments)
       *     Pending Payout Amount
       *     Pending Payout Time
       *     Creation Date
       *
       */
      ///@{




   /**
    *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
    *  returns operations in the range [from-limit, from]
    *
    *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
    *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
    */
      map<uint32_t,operation_object> get_account_history( string account, uint64_t from, uint32_t limit )const;

      vector<balance_object> get_balance_objects( const vector<address>& addrs )const;
      vector<balance_object> get_balance_objects_by_key( const string& pubkey )const;

      ////////////////////////////
      // Handlers - not exposed //
      ////////////////////////////
      void on_api_startup();

   private:
       std::shared_ptr< database_api_impl > my;
};

} }

FC_REFLECT( muse::app::order, (order_price)(real_price)(base)(quote)(created) );
FC_REFLECT( muse::app::order_book, (asks)(bids) );
FC_REFLECT( muse::app::scheduled_hardfork, (hf_version)(live_time) );
FC_REFLECT( muse::app::liquidity_balance, (account)(weight) );

FC_REFLECT( muse::app::discussion_query, (tag)(filter_tags)(start_author)(start_permlink)(parent_author)(parent_permlink)(limit) );

FC_API(muse::app::database_api,
   // Subscriptions
   (set_subscribe_callback)
   (set_pending_transaction_callback)
   (set_block_applied_callback)
   (cancel_all_subscriptions)


   // Blocks and transactions
   (get_block_header)
   (get_block)
//   (get_state)

   (get_proposed_transactions)

   // Globals
   (get_config)
   (get_objects)
   (get_dynamic_global_properties)
   (get_chain_properties)
   (get_feed_history)
   (get_current_median_history_price)
   (get_witness_schedule)
   (get_hardfork_version)
   (get_next_scheduled_hardfork)

   // Keys
   (get_key_references)

   // Accounts
   (get_accounts)
   (get_account_from_id)
   (get_account_references)
   (lookup_account_names)
   (lookup_accounts)
   (get_account_count)
   (get_conversion_requests)
   (get_account_history)
   (get_owner_history)
   (get_recovery_request)

   // Market
   (get_order_book)
   (get_order_book_for_asset)
   (get_open_orders)
   (get_liquidity_queue)

   // Authority / validation
   (get_transaction_hex)
   (get_transaction)
   (get_required_signatures)
   (get_potential_signatures)
   (verify_authority)
   (verify_account_authority)

   // Witnesses
   (get_witnesses)
   (get_witness_by_account)
   (get_witnesses_by_vote)
   (lookup_witness_accounts) 
   (is_streaming_platform)
   (get_witness_count) 
   (get_active_witnesses)
   (get_voted_streaming_platforms)
   // Content
   (lookup_streaming_platform_accounts)
   (get_reports_for_account)
   (get_content_by_uploader)
   (get_content_by_url)
   (lookup_content)
   //UIAs
   (lookup_uias)
   (get_uia_details)
   (get_asset)
   (get_uia_balances)
   //score
   (get_account_scoring)
   (get_content_scoring)
   (get_balance_objects)
   (get_balance_objects_by_key)
)

