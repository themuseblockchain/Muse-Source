#pragma once

#include <muse/app/api.hpp>
#include <muse/private_message/private_message_plugin.hpp>
#include <muse/chain/base_objects.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/real128.hpp>
#include <fc/crypto/base58.hpp>

using namespace muse::app;
using namespace muse::chain;
using namespace graphene::utilities;
using namespace std;

namespace muse { namespace wallet {



typedef uint16_t transaction_handle_type;

using namespace muse::private_message;

typedef uint16_t transaction_handle_type;

/**
 * This class takes a variant and turns it into an object
 * of the given type, with the new operator.
 */

object* create_object( const variant& v );

struct memo_data {

   static optional<memo_data> from_string( string str ) {
      try {
         if( str.size() > sizeof(memo_data) && str[0] == '#') {
            auto data = fc::from_base58( str.substr(1) );
            auto m  = fc::raw::unpack<memo_data>( data );
            FC_ASSERT( string(m) == str );
            return m;
         }
      } catch ( ... ) {}
      return optional<memo_data>();
   }

   public_key_type from;
   public_key_type to;
   uint64_t        nonce = 0;
   uint32_t        check = 0;
   vector<char>    encrypted;

   operator string()const {
      auto data = fc::raw::pack(*this);
      auto base58 = fc::to_base58( data );
      return '#'+base58;
   }
};



struct brain_key_info
{
   string               brain_priv_key;
   public_key_type      pub_key;
   string               wif_priv_key;
};

struct wallet_data
{
   vector<char>              cipher_keys; /** encrypted keys */

   string                    ws_server = "ws://localhost:8090";
   string                    ws_user;
   string                    ws_password;
};

struct signed_block_with_info : public signed_block
{
   signed_block_with_info( const signed_block& block );
   signed_block_with_info( const signed_block_with_info& block ) = default;

   block_id_type block_id;
   public_key_type signing_key;
   vector< transaction_id_type > transaction_ids;
};

enum authority_type
{
   owner,
   active,
   basic
};

namespace detail {
class wallet_api_impl;
}

struct operation_detail {
   string               memo;
   string               description;
   operation_object     op;
};

struct approval_delta
{
   vector<string> active_approvals_to_add;
   vector<string> active_approvals_to_remove;
   vector<string> owner_approvals_to_add;
   vector<string> owner_approvals_to_remove;
   vector<string> key_approvals_to_add;
   vector<string> key_approvals_to_remove;
};

/**
 * This wallet assumes it is connected to the database server with a high-bandwidth, low-latency connection and
 * performs minimal caching. This API could be provided locally to be used by a web interface.
 */
class wallet_api
{
   public:
      wallet_api( const wallet_data& initial_data, fc::api<login_api> rapi );
      virtual ~wallet_api();

      bool copy_wallet_file( string destination_filename );


      /** Returns a list of all commands supported by the wallet API.
       *
       * This lists each command, along with its arguments and return types.
       * For more detailed help on a single command, use \c get_help()
       *
       * @returns a multi-line string suitable for displaying on a terminal
       */
      string                              help()const;

      /**
       * Returns info about the current state of the blockchain
       */
      variant                             info();

      /** Returns info such as client version, git version of graphene/fc, version of boost, openssl.
       * @returns compile time info and client and dependencies versions
       */
      variant_object                      about() const;

      /** Returns the information about a block
       *
       * @param num Block num
       *
       * @returns Public block data on the blockchain
       */
      optional<signed_block_with_info>    get_block( uint32_t num );

      /** Return the current price feed history
       *
       * @returns Price feed history data on the blockchain
       */
      feed_history_object                 get_feed_history()const;

      /**
       * Returns the list of streaming_platforms voted in.
       */
      vector<string>                      get_voted_streaming_platforms()const;
      /**
       * Returns the list of witnesses producing blocks in the current round (21 Blocks)
       */
      vector<string>                      get_active_witnesses()const;

      /**
       * Returns the state info associated with the URL
       */
      //app::state                          get_state( string url );

      /**
       *  Gets the account information for all accounts for which this wallet has a private key
       */
      vector<account_object>              list_my_accounts();

      /**
       * Get list of proposed transactions
       */
      vector<proposal_object> get_proposed_transactions( string account_or_content )const;

      /** Lists all accounts registered in the blockchain.
       * This returns a list of all account names and their account ids, sorted by account name.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all accounts,
       * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
       * the last account name returned as the \c lowerbound for the next \c list_accounts() call.
       *
       * @param lowerbound the name of the first account to return.  If the named account does not exist,
       *                   the list will start at the account that comes after \c lowerbound
       * @param limit the maximum number of accounts to return (max: 1000)
       * @returns a list of accounts mapping account names to account ids
       */
      set<string>  list_accounts(const string& lowerbound, uint32_t limit);

      /** Returns the block chain's rapidly-changing properties.
       * The returned object contains information that changes every block interval
       * such as the head block number, the next witness, etc.
       * @see \c get_global_properties() for less-frequently changing properties
       * @returns the dynamic global properties
       */
      dynamic_global_property_object    get_dynamic_global_properties() const;

      /** Returns information about the given account.
       *
       * @param account_name the name of the account to provide information about
       * @returns the public account data stored in the blockchain
       */
      extended_account                  get_account( string account_name ) const;

      /** Returns the current wallet filename.
       *
       * This is the filename that will be used when automatically saving the wallet.
       *
       * @see set_wallet_filename()
       * @return the wallet filename
       */
      string                            get_wallet_filename() const;

      /**
       * Get the WIF private key corresponding to a public key.  The
       * private key must already be in the wallet.
       */
      string                            get_private_key( public_key_type pubkey )const;

      /**
       *  @param role - active | owner | basic | memo
       */
      pair<public_key_type,string>  get_private_key_from_password( string account, string role, string password )const;


      /**
       * Returns transaction by ID.
       */
      annotated_signed_transaction      get_transaction( transaction_id_type trx_id )const;

      /** Checks whether the wallet has just been created and has not yet had a password set.
       *
       * Calling \c set_password will transition the wallet to the locked state.
       * @return true if the wallet is new
       * @ingroup Wallet Management
       */
      bool    is_new()const;

      /** Checks whether the wallet is locked (is unable to use its private keys).
       *
       * This state can be changed by calling \c lock() or \c unlock().
       * @return true if the wallet is locked
       * @ingroup Wallet Management
       */
      bool    is_locked()const;

      /** Locks the wallet immediately.
       * @ingroup Wallet Management
       */
      void    lock();

      /** Unlocks the wallet.
       *
       * The wallet remain unlocked until the \c lock is called
       * or the program exits.
       * @param password the password previously set with \c set_password()
       * @ingroup Wallet Management
       */
      void    unlock(string password);

      /** Sets a new password on the wallet.
       *
       * The wallet must be either 'new' or 'unlocked' to
       * execute this command.
       * @ingroup Wallet Management
       */
      void    set_password(string password);

      /** Dumps all private keys owned by the wallet.
       *
       * The keys are printed in WIF format.  You can import these keys into another wallet
       * using \c import_key()
       * @returns a map containing the private keys, indexed by their public key
       */
      map<public_key_type, string> list_keys();

      /** Returns detailed help on a single API command.
       * @param method the name of the API command you want help with
       * @returns a multi-line string suitable for displaying on a terminal
       */
      string  gethelp(const string& method)const;

      /** Loads a specified Graphene wallet.
       *
       * The current wallet is closed before the new wallet is loaded.
       *
       * @warning This does not change the filename that will be used for future
       * wallet writes, so this may cause you to overwrite your original
       * wallet unless you also call \c set_wallet_filename()
       *
       * @param wallet_filename the filename of the wallet JSON file to load.
       *                        If \c wallet_filename is empty, it reloads the
       *                        existing wallet file
       * @returns true if the specified wallet is loaded
       */
      bool    load_wallet_file(string wallet_filename = "");

      /** Saves the current wallet to the given filename.
       *
       * @warning This does not change the wallet filename that will be used for future
       * writes, so think of this function as 'Save a Copy As...' instead of
       * 'Save As...'.  Use \c set_wallet_filename() to make the filename
       * persist.
       * @param wallet_filename the filename of the new wallet JSON file to create
       *                        or overwrite.  If \c wallet_filename is empty,
       *                        save to the current filename.
       */
      void    save_wallet_file(string wallet_filename = "");

      /** Sets the wallet filename used for future writes.
       *
       * This does not trigger a save, it only changes the default filename
       * that will be used the next time a save is triggered.
       *
       * @param wallet_filename the new filename to use for future saves
       */
      void    set_wallet_filename(string wallet_filename);

      /** Suggests a safe brain key to use for creating your account.
       * \c create_account_with_brain_key() requires you to specify a 'brain key',
       * a long passphrase that provides enough entropy to generate cyrptographic
       * keys.  This function will suggest a suitably random string that should
       * be easy to write down (and, with effort, memorize).
       * @returns a suggested brain_key
       */
      brain_key_info suggest_brain_key()const;

      /** Converts a signed_transaction in JSON form to its binary representation.
       *
       * TODO: I don't see a broadcast_transaction() function, do we need one?
       *
       * @param tx the transaction to serialize
       * @returns the binary form of the transaction.  It will not be hex encoded,
       *          this returns a raw string that may have null characters embedded
       *          in it
       */
      string serialize_transaction(signed_transaction tx) const;

      /** Imports a WIF Private Key into the wallet to be used to sign transactions by an account.
       *
       * example: import_key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
       *
       * @param wif_key the WIF Private Key to import
       */
      bool import_key( string wif_key );

      /** Transforms a brain key to reduce the chance of errors when re-entering the key from memory.
       *
       * This takes a user-supplied brain key and normalizes it into the form used
       * for generating private keys.  In particular, this upper-cases all ASCII characters
       * and collapses multiple spaces into one.
       * @param s the brain key as supplied by the user
       * @returns the brain key in its normalized form
       */
      string normalize_brain_key(string s) const;

      /**
       *  This method will genrate new owner, active, and memo keys for the new account which
       *  will be controlable by this wallet. There is a fee associated with account creation
       *  that is paid by the creator. The current account creation fee can be found with the
       *  'info' wallet command.
       *
       *  @param creator The account creating the new account
       *  @param new_account_name The name of the new account
       *  @param json_meta JSON Metadata associated with the new account
       *  @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction create_account( string creator, string new_account_name, string json_meta, bool broadcast );

      /**
       * This method is used by faucets to create new accounts for other users which must
       * provide their desired keys. The resulting account may not be controllable by this
       * wallet. There is a fee associated with account creation that is paid by the creator.
       * The current account creation fee can be found with the 'info' wallet command.
       *
       * @param creator The account creating the new account
       * @param newname The name of the new account
       * @param json_meta JSON Metadata associated with the new account
       * @param owner public owner key of the new account
       * @param active public active key of the new account
       * @param basic public basic key of the new account
       * @param memo public memo key of the new account
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction create_account_with_keys( string creator,
                                            string newname,
                                            string json_meta,
                                            public_key_type owner,
                                            public_key_type active,
                                            public_key_type basic,
                                            public_key_type memo,
                                            bool broadcast )const;

      /**
       * This method updates the keys of an existing account.
       *
       * @param accountname The name of the account
       * @param json_meta New JSON Metadata to be associated with the account
       * @param owner New public owner key for the account
       * @param active New public active key for the account
       * @param basic New public basic key for the account
       * @param memo New public memo key for the account
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction update_account( string accountname,
                                         string json_meta,
                                         public_key_type owner,
                                         public_key_type active,
                                         public_key_type basic,
                                         public_key_type memo,
                                         bool broadcast )const;


      /**
       * This method updates the key of an authority for an exisiting account.
       * Warning: You can create impossible authorities using this method. The method
       * will fail if you create an impossible owner authority, but will allow impossible
       * active and basic authorities.
       *
       * @param account_name The name of the account whose authority you wish to update
       * @param type The authority type. e.g. owner, active, or basic
       * @param key The public key to add to the authority
       * @param weight The weight the key should have in the authority. A weight of 0 indicates the removal of the key.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      annotated_signed_transaction update_account_auth_key( string account_name, authority_type type, public_key_type key, weight_type weight, bool broadcast );

      /**
       * This method updates the account of an authority for an exisiting account.
       * Warning: You can create impossible authorities using this method. The method
       * will fail if you create an impossible owner authority, but will allow impossible
       * active and basic authorities.
       *
       * @param account_name The name of the account whose authority you wish to update
       * @param type The authority type. e.g. owner, active, or basic
       * @param auth_account The account to add the the authority
       * @param weight The weight the account should have in the authority. A weight of 0 indicates the removal of the account.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      annotated_signed_transaction update_account_auth_account( string account_name, authority_type type, string auth_account, weight_type weight, bool broadcast );

      /**
       * This method updates the weight threshold of an authority for an account.
       * Warning: You can create impossible authorities using this method as well
       * as implicitly met authorities. The method will fail if you create an implicitly
       * true authority and if you create an impossible owner authoroty, but will allow
       * impossible active and basic authorities.
       *
       * @param account_name The name of the account whose authority you wish to update
       * @param type The authority type. e.g. owner, active, or basic
       * @param threshold The weight threshold required for the authority to be met
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction update_account_auth_threshold( string account_name, authority_type type, uint32_t threshold, bool broadcast );

      /**
       * This method updates the account JSON metadata
       *
       * @param account_name The name of the account you wish to update
       * @param json_meta The new JSON metadata for the account. This overrides existing metadata
       * @param broadcast ture if you wish to broadcast the transaction
       */
      annotated_signed_transaction update_account_meta( string account_name, string json_meta, bool broadcast );

      /**
       * This method updates the memo key of an account
       *
       * @param account_name The name of the account you wish to update
       * @param key The new memo public key
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction update_account_memo_key( string account_name, public_key_type key, bool broadcast );

      /**
       * This method creates friend request
       *
       * @param who The name of the account requesting friendship
       * @param whom The name of the account to request with
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction friendship( string who, string whom, bool broadcast );

      /**
       *  This method is used to convert a JSON transaction to its transaction ID.
       */
      transaction_id_type get_transaction_id( const signed_transaction& trx )const { return trx.id(); }

      /** Lists all witnesses registered in the blockchain.
       * This returns a list of all account names that own witnesses, and the associated witness id,
       * sorted by name.  This lists witnesses whether they are currently voted in or not.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all witnesss,
       * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
       * the last witness name returned as the \c lowerbound for the next \c list_witnesss() call.
       *
       * @param lowerbound the name of the first witness to return.  If the named witness does not exist,
       *                   the list will start at the witness that comes after \c lowerbound
       * @param limit the maximum number of witnesss to return (max: 1000)
       * @returns a list of witnesss mapping witness names to witness ids
       */
      set<string>       list_witnesses(const string& lowerbound, uint32_t limit);

      /** Lists all streaming_platforms registered in the blockchain.
       * This returns a list of all account names that own streaming_platform,
       * sorted by name.  This lists streaming_platforms whether they are currently voted in or not.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all streaming_platforms,
       * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
       * the last witness name returned as the \c lowerbound for the next \c list_streaming_platforms() call.
       *
       * @param lowerbound the name of the first streaming_platform to return.  If the name does not exist,
       *                   the list will start at the streaming_platform that comes after \c lowerbound
       * @param limit the maximum number of streaming_platforms to return (max: 1000)
       * @returns a list of streaming_platforms mapping witness names to witness ids
       */
      set<string>       list_streaming_platforms(const string& lowerbound, uint32_t limit);

      /** Returns information about the given witness.
       * @param owner_account the name or id of the witness account owner, or the id of the witness
       * @returns the information about the witness stored in the block chain
       */
      optional< witness_object > get_witness(string owner_account);

      /** Returns conversion requests by an account
       *
       * @param owner Account name of the account owning the requests
       *
       * @returns All pending conversion requests by account
       */
      vector<convert_request_object> get_conversion_requests( string owner );


      /**
       * Update a witness object owned by the given account.
       *
       * @param witness_name The name of the witness's owner account.  Also accepts the ID of the owner account or the ID of the witness.
       * @param url Same as for create_witness.  The empty string makes it remain the same.
       * @param block_signing_key The new block signing public key.  The empty string makes it remain the same.
       * @param props The chain properties the witness is voting on.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      annotated_signed_transaction update_witness(string witness_name,
                                        string url,
                                        public_key_type block_signing_key,
                                        const chain_properties& props,
                                        bool broadcast = false);

      /**
       * Update streaming_platform.
       *
       * @param streaming_platform_name The name of the account to be upgraded.
       * @param url Url used for promotion.
       * @param broadcast true if you wish to broadcast the transaction.
       *
       */
      annotated_signed_transaction update_streaming_platform(string streaming_platform_name,
                                        string url, bool broadcast = false);

      /** Set the voting proxy for an account.
       *
       * If a user does not wish to take an active part in voting, they can choose
       * to allow another account to vote their stake.
       *
       * Setting a vote proxy does not remove your previous votes from the blockchain,
       * they remain there but are ignored.  If you later null out your vote proxy,
       * your previous votes will take effect again.
       *
       * This setting can be changed at any time.
       *
       * @param account_to_modify the name or id of the account to update
       * @param proxy the name of account that should proxy to, or empty string to have no proxy
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction set_voting_proxy(string account_to_modify,
                                          string proxy,
                                          bool broadcast = false);

      /**
       * Vote for a witness to become a block producer. By default an account has not voted
       * positively or negatively for a witness. The account can either vote for with positively
       * votes or against with negative votes. The vote will remain until updated with another
       * vote. Vote strength is determined by the accounts vesting shares.
       *
       * @param account_to_vote_with The account voting for a witness
       * @param witness_to_vote_for The witness that is being voted for
       * @param approve true if the account is voting for the account to be able to be a block produce
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction vote_for_witness(string account_to_vote_with,
                                          string witness_to_vote_for,
                                          bool approve = true,
                                          bool broadcast = false);

      /**
       * Vote for a streaming platform to allow it to produce usage reports. By default an account has not voted
       * positively or negatively for a witness. The account can either vote for with positively
       * votes or against with negative votes. The vote will remain until updated with another
       * vote. Vote strength is determined by the accounts vesting shares.
       *
       * @param account_to_vote_with The account voting for a witness
       * @param streaming_platform_to_vote_for The witness that is being voted for
       * @param approve true if the account is voting for the account to be able to be a block produce
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction vote_for_streaming_platform(string account_to_vote_with,
                                          string streaming_platform_to_vote_for,
                                          bool approve = true,
                                          bool broadcast = false);


      /**
       * Transfer funds from one account to another. MUSE and MBD can be transferred.
       *
       * @param from The account the funds are coming from
       * @param to The account the funds are going to
       * @param amount The funds being transferred. i.e. "100.000 MUSE"
       * @param memo A memo for the transactionm, encrypted with the to account's public memo key
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction transfer(string from, string to, asset amount, string memo, bool broadcast = false);

      /**
       * Transfer MUSE into a vesting fund represented by vesting shares (VESTS). VESTS are required to vesting
       * for a minimum of one coin year and can be withdrawn once a week over a two year withdraw period.
       * VESTS are protected against dilution up until 90% of MUSE is vesting.
       *
       * @param from The account the MUSE is coming from
       * @param to The account getting the VESTS
       * @param amount The amount of MUSE to vest i.e. "100.00 MUSE"
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction transfer_to_vesting(string from, string to, asset amount, bool broadcast = false);

      /**
       * Set up a vesting withdraw request. The request is fulfilled once a week over the next two year (104 weeks).
       *
       * @param from The account the VESTS are withdrawn from
       * @param vesting_shares The amount of VESTS to withdraw over the next two years. Each week (amount/104) shares are
       *    withdrawn and deposited back as MUSE. i.e. "10.000000 VESTS"
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction withdraw_vesting( string from, asset vesting_shares, bool broadcast = false );

      /**
       * Set up a vesting withdraw route. When vesting shares are withdrawn, they will be routed to these accounts
       * based on the specified weights.
       *
       * @param from The account the VESTS are withdrawn from.
       * @param to   The account receiving either VESTS or MUSE.
       * @param percent The percent of the withdraw to go to the 'to' account. This is denoted in hundreths of a percent.
       *    i.e. 100 is 1% and 10000 is 100%. This value must be between 1 and 100000
       * @param auto_vest Set to true if the from account should receive the VESTS as VESTS, or false if it should receive
       *    them as MUSE.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      annotated_signed_transaction set_withdraw_vesting_route( string from, string to, uint16_t percent, bool auto_vest, bool broadcast = false );

      /**
       *  This method will convert MBD to MUSE at the current_median_history price one
       *  week from the time it is executed. This method depends upon there being a valid price feed.
       *
       *  @param from The account requesting conversion of its MBD i.e. "1.000 MBD"
       *  @param amount The amount of MBD to convert
       *  @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction convert_sbd( string from, asset amount, bool broadcast = false );

      /**
       * A witness can public a price feed for the MUSE:MBD market. The median price feed is used
       * to process conversion requests from MBD to MUSE.
       *
       * @param witness The witness publishing the price feed
       * @param exchange_rate The desired exchange rate
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction publish_feed(string witness, price exchange_rate, bool broadcast );

      /** Signs a transaction.
       *
       * Given a fully-formed transaction that is only lacking signatures, this signs
       * the transaction with the necessary keys and optionally broadcasts the transaction
       * @param tx the unsigned transaction
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed version of the transaction
       */
      annotated_signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false);

      /** Returns an uninitialized object representing a given blockchain operation.
       *
       * This returns a default-initialized object of the given type; it can be used
       * during early development of the wallet when we don't yet have custom commands for
       * creating all of the operations the blockchain supports.
       *
       * Any operation the blockchain supports can be created using the transaction builder's
       * \c add_operation_to_builder_transaction() , but to do that from the CLI you need to
       * know what the JSON form of the operation looks like.  This will give you a template
       * you can fill in.  It's better than nothing.
       *
       * @param operation_type the type of operation to return, must be one of the
       *                       operations defined in `muse/chain/operations.hpp`
       *                       (e.g., "global_parameters_update_operation")
       * @return a default-constructed operation of the given type
       */
      operation get_prototype_operation(string operation_type);

      void network_add_nodes( const vector<string>& nodes );
      vector< variant > network_get_connected_peers();

      /**
       * Gets the current order book for MUSE:MBD
       *
       * @param limit Maximum number of orders to return for bids and asks. Max is 1000.
       */
      order_book  get_order_book( uint32_t limit = 1000 );
      vector<extended_limit_order>  get_open_orders( string accountname );

      /**
       *  Creates a limit order at the price amount_to_sell / min_to_receive and will deduct amount_to_sell from account
       *
       *  @param owner The name of the account creating the order
       *  @param order_id is a unique identifier assigned by the creator of the order, it can be reused after the order has been filled
       *  @param amount_to_sell The amount of either MBD or MUSE you wish to sell
       *  @param min_to_receive The amount of the other asset you will receive at a minimum
       *  @param fill_or_kill true if you want the order to be killed if it cannot immediately be filled
       *  @param expiration the time the order should expire if it has not been filled
       *  @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction create_order( string owner, uint32_t order_id, asset amount_to_sell, asset min_to_receive, bool fill_or_kill, uint32_t expiration, bool broadcast );

      /**
       * Cancel an order created with create_order
       *
       * @param owner The name of the account owning the order to cancel_order
       * @param orderid The unique identifier assigned to the order by its creator
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction cancel_order( string owner, uint32_t orderid, bool broadcast );


      /**
       * Post a new content. TODO_MUSE update this
       * @param uploader the creating account
       * @param url URL of the content
       * @param title track title of the content
       * @param distributor Vector with distributions
       * @param management Vector with management votes distribution
       * @param threshold Management voting threshold
       * @param playing_reward Reward for the streaming platform
       * @param publishers_share Share for the publishers, in BP; master_share = 10000bp - publishers_share
       * @param third_party_publishers Are there publishers independent to master side?
       * @param distributor_publishing Vector with distributions on publishers' side
       * @param management_publishing Management vector for the publishing settings. Must be non-empty if third_party_publishers = true
       * @param threshold_publishing Management voting threshold
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction create_content(string uploader, string url, string title, vector<distribution> distributor,
                                                  vector<management_vote> management, uint32_t threshold,
                                                  uint32_t playing_reward, uint32_t publishers_share,
                                                  bool third_party_publishers, vector<distribution> distributor_publishing,
                                                  vector<management_vote> management_publishing,
                                                  uint32_t threshold_publishing, bool broadcast);
      
      /**
       * Approve content submission
       * @param approver Approver's account
       * @param url URL of the content being approved
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction approve_content( string approver, string url,  bool broadcast );

      /**
       * Update existing content object master settings.
       * @param url url of the content
       * @param new_title New title or empty string
       * @param new_distributor Vector with new distributions
       * @param new_management Vector with new management votes distribution
       * @param new_threshold New management voting threshold
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction update_content_master(string url, string new_title, vector<distribution> new_distributor,
                                                         vector<management_vote> new_management, uint32_t new_threshold,
                                                         bool broadcast);

      /**
       * Update existing content object publisher settings.
       * @param url url of the content
       * @param new_title New title or empty string
       * @param new_distributor Vector with new distributions
       * @param new_management Vector with new management votes distribution
       * @param new_threshold New management voting threshold
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction update_content_publishing(string url, string new_title, vector<distribution> new_distributor,
                                                                   vector<management_vote> new_management, uint32_t new_threshold, bool broadcast);

      /**
       * Update existing content object publisher settings.
       * @param url url of the content
       * @param new_publishers_share New share for publishers
       * @param new_playing_reward Reward for streaming platforms
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction update_content_global(string url, uint32_t new_publishers_share, uint32_t new_playing_reward, bool broadcast);


      /**
       * Delete existing content object
       * @param publisher the creating account
       * @param url URL of the content
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction remove_content( string url, bool broadcast );

      /**
       * Get reports for a given consumer.
       * @param consumer - consumer's report to look for
       */
      vector <report_object> get_reports(string consumer);

      /**
       * Get score of the given account
       * @param account Account to look for
       * @return Score
       */
      uint64_t get_account_scoring( string account );
      /**
       * Get score of the given content
       * @param content Content to look for
       * @return Score
       */
      uint64_t get_content_scoring( string content );

      /**
       * Create a new UIA.
       * @param asset_name Unique asset name
       * @param description Description
       * @param max_supply Maximum supply
       * @param broadcast Broadcast the transaction?
       */
      annotated_signed_transaction create_asset(string issuer, string asset_name, string description, uint64_t max_supply, bool broadcast);
      
      /**
       * Issue asset to an account
       * @param asset_name Asset to be issued
       * @param to_account Account to which the asset is issued
       * @param amount Issued amount
       * @param broadcast Broadcast the transaction?
       */
      annotated_signed_transaction issue_asset(string asset_name, string to_account, fc::real128 amount, bool broadcast);
      
      /**
       * Reserve asset from an account
       * @param asset_name Asset to be reserved
       * @param from_account Account from which the asset is removed
       * @param amount Reserved amount
       * @param broadcast Broadcast the transaction?
       */
      annotated_signed_transaction reserve_asset(string asset_name, string from_account, fc::real128 amount, bool broadcast);

      /**
       * Update an UIA.
       * @param asset_name Asset to be updated
       * @param description Description
       * @param max_supply Maximum supply
       * @param new_issuer Transfer asset to new issuer
       * @param broadcast Broadcast the transaction?
       */      
      annotated_signed_transaction update_asset(string asset_name, string description, uint64_t max_supply, string new_issuer, bool broadcast);

      /**
       * Get content submitted by given account
       * @param account Account submitting the content
       */
      vector<content_object> get_content_by_account(string account);

      /**
       * Get content with given URL
       * @param url identifier
       */
      optional<content_object>    get_content_by_url(string url);

      /**
       * Get content list, by name
       * @param start Starting name
       * @param limit Limit, less than 1000
       */
      vector<content_object>  lookup_content(const string& start, uint32_t limit );
      
      /**
       * Get content list, by namei, filtered by approver
       * @param start Starting name
       * @param limit Limit, less than 1000
       */
      vector<content_object>  lookup_content_by_approver(const string& start, const string& approver, uint32_t limit );
      /**
       * Get asset details
       */
      optional<asset_object>  get_asset_details(string asset_name);
      vector<asset_object> list_assets( uint32_t start_id, uint32_t limit); 
      vector <account_balance_object> get_assets (string account);
      annotated_signed_transaction      send_private_message( string from, string to, string subject, string body, bool broadcast );
      annotated_signed_transaction      post_streaming_report( string streaming_platform, string consumer, string content, string playlist_creator, uint64_t playtime, bool broadcast );
      
      vector<extended_message_object>   get_inbox( string account, fc::time_point newest, uint32_t limit );
      vector<extended_message_object>   get_outbox( string account, fc::time_point newest, uint32_t limit );
      message_body try_decrypt_message( const message_object& mo );

      /**
       * Vote on a comment to be paid MUSE
       *
       * @param voter The account voting
       * @param author The author of the comment to be voted on
       * @param url The permlink of the comment to be voted on. (author, permlink) is a unique pair
       * @param weight The weight [-100,100] of the vote
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction vote(string voter, string url, int16_t weight, bool broadcast);

      /**
       * Sets the amount of time in the future until a transaction expires.
       */
      void set_transaction_expiration(uint32_t seconds);

      /**
       * Challenge a user's authority. The challenger pays a fee to the challenged which is depositted as
       * VEST. Until the challenged proves their active key, all basic rights are revoked.
       *
       * @param challenger The account issuing the challenge
       * @param challenged The account being challenged
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction challenge( string challenger, string challenged, bool broadcast );

      /**
       * Create an account recovery request as a recover account. The syntax for this command contains a serialized authority object
       * so there is an example below on how to pass in the authority.
       *
       * request_account_recovery "your_account" "account_to_recover" {"weight_threshold": 1,"account_auths": [], "key_auths": [["new_public_key",1]]} true
       *
       * @param recovery_account The name of your account
       * @param account_to_recover The name of the account you are trying to recover
       * @param new_authority The new owner authority for the recovered account. This should be given to you by the holder of the compromised or lost account.
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction request_account_recovery( string recovery_account, string account_to_recover, authority new_authority, bool broadcast );

      /**
       * Recover your account using a recovery request created by your recovery account. The syntax for this commain contains a serialized
       * authority object, so there is an example below on how to pass in the authority.
       *
       * recover_account "your_account" {"weight_threshold": 1,"account_auths": [], "key_auths": [["old_public_key",1]]} {"weight_threshold": 1,"account_auths": [], "key_auths": [["new_public_key",1]]} true
       *
       * @param account_to_recover The name of your account
       * @param recent_authority A recent owner authority on your account
       * @param new_authority The new authority that your recovery account used in the account recover request.
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction recover_account( string account_to_recover, authority recent_authority, authority new_authority, bool broadcast );

      /**
       * Change your recovery account after a 30 day delay.
       *
       * @param owner The name of your account
       * @param new_recovery_account The name of the recovery account you wish to have
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction change_recovery_account( string owner, string new_recovery_account, bool broadcast );

      vector< owner_authority_history_object > get_owner_history( string account )const;

      /**
       * Prove an account's active authority, fulfilling a challenge, restoring basic rights, and making
       * the account immune to challenge for 24 hours.
       *
       * @param challenged The account that was challenged and is proving its authority.
       * @param broadcast true if you wish to broadcast the transaction
       */
      annotated_signed_transaction prove( string challenged, bool broadcast );

      /**
       *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
       *  returns operations in the range [from-limit, from]
       *
       *  @param account - account whose history will be returned
       *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
       *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
       */
      map<uint32_t,operation_object> get_account_history( string account, uint32_t from, uint32_t limit );

      /**
       * @ingroup Transaction Builder API
       */
      transaction_handle_type begin_builder_transaction();
      /**
       * @ingroup Transaction Builder API
       */
      void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op);
      /**
       * @ingroup Transaction Builder API
       */
      void replace_operation_in_builder_transaction(transaction_handle_type handle,
            unsigned operation_index,
            const operation& new_op);
      /**
       * @ingroup Transaction Builder API
       */
      transaction preview_builder_transaction(transaction_handle_type handle);
      /**
       * @ingroup Transaction Builder API
       */
      signed_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true);
      /**
       * @ingroup Transaction Builder API
       */
      signed_transaction propose_builder_transaction(
            transaction_handle_type handle,
            time_point_sec expiration = time_point::now() + fc::minutes(1),
            uint32_t review_period_seconds = 0,
            bool broadcast = true
            );

      signed_transaction propose_builder_transaction2(
            transaction_handle_type handle,
            string account_name_or_id,
            time_point_sec expiration = time_point::now() + fc::minutes(1),
            uint32_t review_period_seconds = 0,
            bool broadcast = true
            );

      /**
       * @ingroup Transaction Builder API
       */
      void remove_builder_transaction(transaction_handle_type handle);


      /** Approve or disapprove a proposal.
       *
       * @param fee_paying_account The account paying the fee for the op.
       * @param proposal_id The proposal to modify.
       * @param delta Members contain approvals to create or remove.  In JSON you can leave empty members undefined.
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed version of the transaction
       */
      signed_transaction approve_proposal(
            const string& proposal_id,
            const approval_delta& delta,
            bool broadcast /* = false */
            );

      std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const;

      fc::signal<void(bool)> lock_changed;
      std::shared_ptr<detail::wallet_api_impl> my;
      void encrypt_keys();
      annotated_signed_transaction  import_balance( string name_or_id, const vector<string>& wif_keys, bool broadcast );
      vector<balance_object> get_balance_objects( const string& pub_key );
};

struct plain_keys {
   fc::sha512                  checksum;
   map<public_key_type,string> keys;
};

} }

FC_REFLECT( muse::wallet::wallet_data,
            (cipher_keys)
            (ws_server)
            (ws_user)
            (ws_password)
          )

FC_REFLECT( muse::wallet::brain_key_info, (brain_priv_key)(wif_priv_key) (pub_key));

FC_REFLECT_DERIVED( muse::wallet::signed_block_with_info, (muse::chain::signed_block),
   (block_id)(signing_key)(transaction_ids) )

FC_REFLECT( muse::wallet::operation_detail, (memo)(description)(op) )
FC_REFLECT( muse::wallet::plain_keys, (checksum)(keys) )

FC_REFLECT_ENUM( muse::wallet::authority_type, (owner)(active)(basic) )

FC_API( muse::wallet::wallet_api,
        /// wallet api
        (help)(gethelp)
        (about)(is_new)(is_locked)(lock)(unlock)(set_password)
        (load_wallet_file)(save_wallet_file)

        /// key api
        (import_key)
        (suggest_brain_key)
        (list_keys)
        (get_private_key)
        (get_private_key_from_password)
        (normalize_brain_key)

        /// query api
        (info)
        (list_my_accounts)
        (list_accounts)
        (list_witnesses)
        (list_streaming_platforms)
        (get_witness)
        (get_account)
        (get_block)
        (get_feed_history)
        (get_conversion_requests)
        (get_account_history)
        //(get_state)
        (get_proposed_transactions)

        /// transaction api
        (create_account)
        (create_account_with_keys)
        (update_account)
        (update_account_auth_key)
        (update_account_auth_account)
        (update_account_auth_threshold)
        (update_account_meta)
        (update_account_memo_key)
        (update_witness)
        (update_streaming_platform)
        (set_voting_proxy)
        (vote_for_witness)
        (vote_for_streaming_platform)
        (transfer)
        (transfer_to_vesting)
        (withdraw_vesting)
        (set_withdraw_vesting_route)
        (convert_sbd)
        (publish_feed)
        (get_order_book)
        (get_open_orders)
        (create_order)
        (cancel_order)
        (create_content)
        (approve_content)
        (update_content_master)
        (update_content_publishing)
        (update_content_global)
        (remove_content)
        (get_content_by_account)
        (get_content_by_url)
        (lookup_content)
        //(lookup_content_by_approver)
        (post_streaming_report)
        (get_reports)
        (vote)
        (set_transaction_expiration)
        (challenge)
        (prove)
        (request_account_recovery)
        (recover_account)
        (change_recovery_account)
        (get_owner_history)
        (create_asset)
        (issue_asset)
        (reserve_asset)
        (update_asset)
        (get_asset_details)
        (list_assets)
        (get_assets)

        // private message api
        (send_private_message)
        (get_inbox)
        (get_outbox)

        /// helper api
        (get_prototype_operation)
        (serialize_transaction)
        (sign_transaction)

        (network_add_nodes)
        (network_get_connected_peers)

        (get_active_witnesses)
        (get_voted_streaming_platforms)
        (get_transaction)

        (approve_proposal)
        (begin_builder_transaction)
        (add_operation_to_builder_transaction)
        (replace_operation_in_builder_transaction)
        (preview_builder_transaction)
        (sign_builder_transaction)
        (propose_builder_transaction)
        (propose_builder_transaction2)
        (remove_builder_transaction)
        (friendship)
        (get_account_scoring)
        (get_content_scoring)
        (import_balance)
        (get_balance_objects)
      )

FC_REFLECT( muse::wallet::approval_delta,
        (active_approvals_to_add)
        (active_approvals_to_remove)
        (owner_approvals_to_add)
        (owner_approvals_to_remove)
        (key_approvals_to_add)
        (key_approvals_to_remove)
      )
FC_REFLECT( muse::wallet::memo_data, (from)(to)(nonce)(check)(encrypted) );
