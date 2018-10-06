#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <list>
#include <locale>

#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/sort.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <fc/container/deque.hpp>
#include <fc/git_revision.hpp>
#include <fc/real128.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <graphene/utilities/git_revision.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/words.hpp>

#include <muse/app/api.hpp>
#include <muse/chain/protocol/base.hpp>
#include <muse/wallet/wallet.hpp>
#include <muse/wallet/api_documentation.hpp>
#include <muse/wallet/reflect_util.hpp>
#include <fc/smart_ref_impl.hpp>

#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
#endif

#define BRAIN_KEY_WORD_COUNT 16

static const std::locale& c_locale = std::locale::classic();

namespace muse { namespace wallet {

namespace detail {

template<class T>
optional<T> maybe_id( const string& name_or_id )
{
   if( std::isdigit( name_or_id.front(), c_locale ) )
   {
      try
      {
         return fc::variant(name_or_id, 1).as<T>(1);
      }
      catch (const fc::exception&)
      { // not an ID
      }
   }
   return optional<T>();
}

string pubkey_to_shorthash( const public_key_type& key )
{
   uint32_t x = fc::sha256::hash(key)._hash[0];
   static const char hd[] = "0123456789abcdef";
   string result;

   result += hd[(x >> 0x1c) & 0x0f];
   result += hd[(x >> 0x18) & 0x0f];
   result += hd[(x >> 0x14) & 0x0f];
   result += hd[(x >> 0x10) & 0x0f];
   result += hd[(x >> 0x0c) & 0x0f];
   result += hd[(x >> 0x08) & 0x0f];
   result += hd[(x >> 0x04) & 0x0f];
   result += hd[(x        ) & 0x0f];

   return result;
}


fc::ecc::private_key derive_private_key( const std::string& prefix_string,
                                         int sequence_number )
{
   std::string sequence_string = std::to_string(sequence_number);
   fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
   fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
   return derived_key;
}

string normalize_brain_key( string s )
{
   size_t i = 0, n = s.length();
   std::string result;
   char c;
   result.reserve( n );

   bool preceded_by_whitespace = false;
   bool non_empty = false;
   while( i < n )
   {
      c = s[i++];
      switch( c )
      {
      case ' ':  case '\t': case '\r': case '\n': case '\v': case '\f':
         preceded_by_whitespace = true;
         continue;

      case 'a': c = 'A'; break;
      case 'b': c = 'B'; break;
      case 'c': c = 'C'; break;
      case 'd': c = 'D'; break;
      case 'e': c = 'E'; break;
      case 'f': c = 'F'; break;
      case 'g': c = 'G'; break;
      case 'h': c = 'H'; break;
      case 'i': c = 'I'; break;
      case 'j': c = 'J'; break;
      case 'k': c = 'K'; break;
      case 'l': c = 'L'; break;
      case 'm': c = 'M'; break;
      case 'n': c = 'N'; break;
      case 'o': c = 'O'; break;
      case 'p': c = 'P'; break;
      case 'q': c = 'Q'; break;
      case 'r': c = 'R'; break;
      case 's': c = 'S'; break;
      case 't': c = 'T'; break;
      case 'u': c = 'U'; break;
      case 'v': c = 'V'; break;
      case 'w': c = 'W'; break;
      case 'x': c = 'X'; break;
      case 'y': c = 'Y'; break;
      case 'z': c = 'Z'; break;

      default:
         break;
      }
      if( preceded_by_whitespace && non_empty )
         result.push_back(' ');
      result.push_back(c);
      preceded_by_whitespace = false;
      non_empty = true;
   }
   return result;
}

struct op_prototype_visitor
{
   typedef void result_type;

   int t = 0;
   flat_map< std::string, operation >& name2op;

   op_prototype_visitor(
      int _t,
      flat_map< std::string, operation >& _prototype_ops
      ):t(_t), name2op(_prototype_ops) {}

   template<typename Type>
   result_type operator()( const Type& op )const
   {
      string name = fc::get_typename<Type>::name();
      size_t p = name.rfind(':');
      if( p != string::npos )
         name = name.substr( p+1 );
      name2op[ name ] = Type();
   }
};

class wallet_api_impl
{
   public:
      api_documentation method_documentation;
   private:
      void enable_umask_protection() {
#ifdef __unix__
         _old_umask = umask( S_IRWXG | S_IRWXO );
#endif
      }

      void disable_umask_protection() {
#ifdef __unix__
         umask( _old_umask );
#endif
      }

      void init_prototype_ops()
      {
         operation op;
         for( int t=0; t<op.count(); t++ )
         {
            op.set_which( t );
            op.visit( op_prototype_visitor(t, _prototype_ops) );
         }
         return;
      }

public:
   wallet_api& self;
   wallet_api_impl( wallet_api& s, const wallet_data& initial_data, fc::api<login_api> rapi )
      : self( s ),
        _remote_api( rapi ),
        _remote_db( rapi->get_api_by_name("database_api")->as< database_api >() ),
        _remote_net_broadcast( rapi->get_api_by_name("network_broadcast_api")->as< network_broadcast_api >() )
   {
      init_prototype_ops();

      _wallet.ws_server = initial_data.ws_server;
      _wallet.ws_user = initial_data.ws_user;
      _wallet.ws_password = initial_data.ws_password;
   }
   virtual ~wallet_api_impl()
   {}

   void encrypt_keys()
   {
      if( !is_locked() )
      {
         plain_keys data;
         data.keys = _keys;
         data.checksum = _checksum;
         auto plain_txt = fc::raw::pack_to_vector(data);
         _wallet.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
      }
   }

   bool copy_wallet_file( string destination_filename )
   {
      fc::path src_path = get_wallet_filename();
      if( !fc::exists( src_path ) )
         return false;
      fc::path dest_path = destination_filename + _wallet_filename_extension;
      int suffix = 0;
      while( fc::exists(dest_path) )
      {
         ++suffix;
         dest_path = destination_filename + "-" + to_string( suffix ) + _wallet_filename_extension;
      }
      wlog( "backing up wallet ${src} to ${dest}",
            ("src", src_path)
            ("dest", dest_path) );

      fc::path dest_parent = fc::absolute(dest_path).parent_path();
      try
      {
         enable_umask_protection();
         if( !fc::exists( dest_parent ) )
            fc::create_directories( dest_parent );
         fc::copy( src_path, dest_path );
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
      return true;
   }

   bool is_locked()const
   {
      return _checksum == fc::sha512();
   }

   variant info() const
   {
      auto dynamic_props = _remote_db->get_dynamic_global_properties();
      fc::mutable_variant_object result(fc::variant( dynamic_props, GRAPHENE_MAX_NESTED_OBJECTS ).get_object());
      result["witness_majority_version"] = fc::string( _remote_db->get_witness_schedule().majority_version );
      result["hardfork_version"] = fc::string( _remote_db->get_hardfork_version() );
      result["head_block_num"] = dynamic_props.head_block_number;
      result["head_block_id"] = fc::variant(dynamic_props.head_block_id, 1);
      result["head_block_age"] = fc::get_approximate_relative_time_string(dynamic_props.time,
                                                                          time_point_sec(time_point::now()),
                                                                          " old");
      result["participation"] = (100*dynamic_props.recent_slots_filled.popcount()) / 128.0;
      result["median_mbd_price"] = fc::variant( _remote_db->get_current_median_history_price(), 3 );
      result["account_creation_fee"] = fc::variant( _remote_db->get_chain_properties().account_creation_fee, 2 );
      return result;
   }

   variant_object about() const
   {
      string client_version( graphene::utilities::git_revision_description );
      const size_t pos = client_version.find( '/' );
      if( pos != string::npos && client_version.size() > pos )
         client_version = client_version.substr( pos + 1 );

      fc::limited_mutable_variant_object result( GRAPHENE_MAX_NESTED_OBJECTS );
      result["client_version"]           = client_version;
      result["muse_revision"]            = graphene::utilities::git_revision_sha;
      result["muse_revision_age"]        = fc::get_approximate_relative_time_string( fc::time_point_sec( graphene::utilities::git_revision_unix_timestamp ) );
      result["fc_revision"]              = fc::git_revision_sha;
      result["fc_revision_age"]          = fc::get_approximate_relative_time_string( fc::time_point_sec( fc::git_revision_unix_timestamp ) );
      result["compile_date"]             = "compiled on " __DATE__ " at " __TIME__;
      result["boost_version"]            = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
      result["openssl_version"]          = OPENSSL_VERSION_TEXT;

      std::string bitness = boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit";
#if defined(__APPLE__)
      std::string os = "osx";
#elif defined(__linux__)
      std::string os = "linux";
#elif defined(_MSC_VER)
      std::string os = "win32";
#else
      std::string os = "other";
#endif
      result["build"] = os + " " + bitness;

      return result;
   }

   vector<proposal_object> get_proposed_transactions( string id )const
   {
      return _remote_db->get_proposed_transactions( id );   
   }

   extended_account get_account( string account_name ) const
   {
      auto accounts = _remote_db->get_accounts( { account_name } );
      FC_ASSERT( !accounts.empty(), "Unknown account" );
      return accounts.front();
   }

   optional<account_object> get_account_from_id (account_id_type account_id) const
   {
      auto account = _remote_db->get_account_from_id( { account_id } );
      return account; 
   }

   string get_wallet_filename() const { return _wallet_filename; }

   optional<fc::ecc::private_key>  try_get_private_key(const public_key_type& id)const
   {
      auto it = _keys.find(id);
      if( it != _keys.end() )
         return wif_to_key( it->second );
      return optional<fc::ecc::private_key>();
   }

   fc::ecc::private_key              get_private_key(const public_key_type& id)const
   {
      auto has_key = try_get_private_key( id );
      FC_ASSERT( has_key );
      return *has_key;
   }


   fc::ecc::private_key get_private_key_for_account(const account_object& account)const
   {
      vector<public_key_type> active_keys = account.active.get_keys();
      if (active_keys.size() != 1)
         FC_THROW("Expecting a simple authority with one active key");
      return get_private_key(active_keys.front());
   }

   // imports the private key into the wallet, and associate it in some way (?) with the
   // given account name.
   // @returns true if the key matches a current active/owner/memo key for the named
   //          account, false otherwise (but it is stored either way)
   bool import_key(string wif_key)
   {
      fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
      if (!optional_private_key)
         FC_THROW("Invalid private key");
      muse::chain::public_key_type wif_pub_key = optional_private_key->get_public_key();

      _keys[wif_pub_key] = wif_key;
      return true;
   }

   bool load_wallet_file(string wallet_filename = "")
   {
      // TODO:  Merge imported wallet with existing wallet,
      //        instead of replacing it
      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;

      if( ! fc::exists( wallet_filename ) )
         return false;

      _wallet = fc::json::from_file( wallet_filename ).as< wallet_data >( GRAPHENE_MAX_NESTED_OBJECTS );

      return true;
   }

   void save_wallet_file(string wallet_filename = "")
   {
      //
      // Serialize in memory, then save to disk
      //
      // This approach lessens the risk of a partially written wallet
      // if exceptions are thrown in serialization
      //

      encrypt_keys();

      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;

      wlog( "saving wallet to file ${fn}", ("fn", wallet_filename) );

      string data = fc::json::to_pretty_string( _wallet );
      try
      {
         enable_umask_protection();
         //
         // Parentheses on the following declaration fails to compile,
         // due to the Most Vexing Parse.  Thanks, C++
         //
         // http://en.wikipedia.org/wiki/Most_vexing_parse
         //
         fc::ofstream outfile{ fc::path( wallet_filename ) };
         outfile.write( data.c_str(), data.length() );
         outfile.flush();
         outfile.close();
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
   }

   // This function generates derived keys starting with index 0 and keeps incrementing
   // the index until it finds a key that isn't registered in the block chain.  To be
   // safer, it continues checking for a few more keys to make sure there wasn't a short gap
   // caused by a failed registration or the like.
   int find_first_unused_derived_key_index(const fc::ecc::private_key& parent_key)
   {
      int first_unused_index = 0;
      int number_of_consecutive_unused_keys = 0;
      for (int key_index = 0; ; ++key_index)
      {
         fc::ecc::private_key derived_private_key = derive_private_key(key_to_wif(parent_key), key_index);
         muse::chain::public_key_type derived_public_key = derived_private_key.get_public_key();
         if( _keys.find(derived_public_key) == _keys.end() )
         {
            if (number_of_consecutive_unused_keys)
            {
               ++number_of_consecutive_unused_keys;
               if (number_of_consecutive_unused_keys > 5)
                  return first_unused_index;
            }
            else
            {
               first_unused_index = key_index;
               number_of_consecutive_unused_keys = 1;
            }
         }
         else
         {
            // key_index is used
            first_unused_index = 0;
            number_of_consecutive_unused_keys = 0;
         }
      }
   }

   signed_transaction create_account_with_private_key(fc::ecc::private_key owner_privkey,
                                                      string account_name,
                                                      string creator_account_name,
                                                      bool broadcast = false,
                                                      bool save_wallet = true)
   { try {
         int active_key_index = find_first_unused_derived_key_index(owner_privkey);
         fc::ecc::private_key active_privkey = derive_private_key( key_to_wif(owner_privkey), active_key_index);

         int memo_key_index = find_first_unused_derived_key_index(active_privkey);
         fc::ecc::private_key memo_privkey = derive_private_key( key_to_wif(active_privkey), memo_key_index);

         muse::chain::public_key_type owner_pubkey = owner_privkey.get_public_key();
         muse::chain::public_key_type active_pubkey = active_privkey.get_public_key();
         muse::chain::public_key_type memo_pubkey = memo_privkey.get_public_key();

         account_create_operation account_create_op;

         account_create_op.creator = creator_account_name;
         account_create_op.new_account_name = account_name;
         account_create_op.fee = _remote_db->get_chain_properties().account_creation_fee;
         account_create_op.owner = authority(1, owner_pubkey, 1);
         account_create_op.active = authority(1, active_pubkey, 1);
         account_create_op.memo_key = memo_pubkey;

         signed_transaction tx;

         tx.operations.push_back( account_create_op );
         tx.validate();

         if( save_wallet )
            save_wallet_file();
         if( broadcast )
         {
            //_remote_net_broadcast->broadcast_transaction( tx );
            auto result = _remote_net_broadcast->broadcast_transaction_synchronous( tx );
         }
         return tx;
   } FC_CAPTURE_AND_RETHROW( (account_name)(creator_account_name)(broadcast) ) }

   signed_transaction set_voting_proxy(string account_to_modify, string proxy, bool broadcast /* = false */)
   { try {
      account_witness_proxy_operation op;
      op.account = account_to_modify;
      op.proxy = proxy;

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account_to_modify)(proxy)(broadcast) ) }

   optional< witness_object > get_witness( string owner_account )
   {
      return _remote_db->get_witness_by_account( owner_account );
   }

   void set_transaction_expiration( uint32_t tx_expiration_seconds )
   {
      FC_ASSERT( tx_expiration_seconds < MUSE_MAX_TIME_UNTIL_EXPIRATION );
      _tx_expiration_seconds = tx_expiration_seconds;
   }

   map<transaction_handle_type, signed_transaction> _builder_transactions;

   transaction_handle_type begin_builder_transaction()
   {
      int trx_handle = _builder_transactions.empty()? 0
         : (--_builder_transactions.end())->first + 1;
      _builder_transactions[trx_handle];
      return trx_handle;
   }

   void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op)
   {
      FC_ASSERT(_builder_transactions.count(transaction_handle));
      _builder_transactions[transaction_handle].operations.emplace_back(op);
   }
   
   void replace_operation_in_builder_transaction(transaction_handle_type handle,
         uint32_t operation_index,
         const operation& new_op)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      signed_transaction& trx = _builder_transactions[handle];
      FC_ASSERT( operation_index < trx.operations.size());
      trx.operations[operation_index] = new_op;
   }

   vector<content_object> get_content_by_account(string account);
   optional<content_object>    get_content_by_url(string url);
   vector<content_object>  lookup_content(const string& start, uint32_t limit );
   annotated_signed_transaction  import_balance( string name_or_id, const vector<string>& wif_keys, bool broadcast );
  
   transaction preview_builder_transaction(transaction_handle_type handle)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      return _builder_transactions[handle];
   }

   signed_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true)
   {
      FC_ASSERT(_builder_transactions.count(transaction_handle));

      return _builder_transactions[transaction_handle] = sign_transaction(_builder_transactions[transaction_handle], broadcast);
   }
   
   signed_transaction propose_builder_transaction(
         transaction_handle_type handle,
         time_point_sec expiration = time_point::now() + fc::minutes(1),
         uint32_t review_period_seconds = 0, bool broadcast = true)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      proposal_create_operation op;
      op.expiration_time = expiration;
      signed_transaction& trx = _builder_transactions[handle];
      std::transform(trx.operations.begin(), trx.operations.end(), std::back_inserter(op.proposed_ops),
            [](const operation& op) -> op_wrapper { return op; });
      if( review_period_seconds )
         op.review_period_seconds = review_period_seconds;
      trx.operations = {op};

      return trx = sign_transaction(trx, broadcast);
   }

   signed_transaction propose_builder_transaction2(
         transaction_handle_type handle,
         string account_name_or_id,
         time_point_sec expiration = time_point::now() + fc::minutes(1),
         uint32_t review_period_seconds = 0, bool broadcast = true)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      proposal_create_operation op;
      op.expiration_time = expiration;
      signed_transaction& trx = _builder_transactions[handle];
      std::transform(trx.operations.begin(), trx.operations.end(), std::back_inserter(op.proposed_ops),
            [](const operation& op) -> op_wrapper { return op; });
      if( review_period_seconds )
         op.review_period_seconds = review_period_seconds;
      trx.operations = {op};

      return trx = sign_transaction(trx, broadcast);
   }

   template<typename T>
      T get_object(object_id<T::space_id, T::type_id, T> id)const
      {
         auto ob = _remote_db->get_objects({id}).front();
         return ob.template as<T>();
      }

   signed_transaction approve_proposal(
         const string& proposal_id,
         const approval_delta& delta,
         bool broadcast = false)
   {
      proposal_update_operation update_op;

      update_op.proposal = fc::variant( proposal_id, 1 ).as<proposal_id_type>( 1 );

      for( const std::string& name : delta.active_approvals_to_add )
         update_op.active_approvals_to_add.insert( name );
      for( const std::string& name : delta.active_approvals_to_remove )
         update_op.active_approvals_to_remove.insert( name );
      for( const std::string& name : delta.owner_approvals_to_add )
         update_op.owner_approvals_to_add.insert( name );
      for( const std::string& name : delta.owner_approvals_to_remove )
         update_op.owner_approvals_to_remove.insert( name );
      for( const std::string& k : delta.key_approvals_to_add )
         update_op.key_approvals_to_add.insert( public_key_type( k ) );
      for( const std::string& k : delta.key_approvals_to_remove )
         update_op.key_approvals_to_remove.insert( public_key_type( k ) );

      signed_transaction tx;
      tx.operations.push_back(update_op);
      tx.validate();
      return sign_transaction(tx, broadcast);
   }

   void remove_builder_transaction(transaction_handle_type handle)
   {
      _builder_transactions.erase(handle);
   }


   annotated_signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false)
   {
      flat_set< string >   req_active_approvals;
      flat_set< string >   req_owner_approvals;
      flat_set< string >   req_basic_approvals;
      flat_set< string >   req_content_approvals;
      flat_set< string >   req_comp_content_approvals;
      vector< authority >  other_auths;

      tx.get_required_authorities( req_active_approvals, req_owner_approvals, req_basic_approvals, req_content_approvals, req_comp_content_approvals, other_auths );

      for( const auto& auth : other_auths )
         for( const auto& a : auth.account_auths )
            req_active_approvals.insert(a.first);

      for( const string& con : req_content_approvals )
      {
         optional<content_object> content = *_remote_db->get_content_by_url ( con );
         for( const auto& a : content->manage_master.account_auths )
            req_active_approvals.insert(a.first);
      }

      for( const string& con : req_comp_content_approvals )
      {
         optional<content_object> content = *_remote_db->get_content_by_url ( con );
         for( const auto& a : content->manage_comp.account_auths )
            req_active_approvals.insert(a.first);
      }

      // std::merge lets us de-duplicate account_id's that occur in both
      //   sets, and dump them into a vector (as required by remote_db api)
      //   at the same time
      vector< string > v_approving_account_names;
      std::merge(req_active_approvals.begin(), req_active_approvals.end(),
                 req_owner_approvals.begin() , req_owner_approvals.end(),
                 std::back_inserter( v_approving_account_names ) );

      for( const auto& a : req_basic_approvals )
         v_approving_account_names.push_back(a);

      /// TODO: fetch the accounts specified via other_auths as well.

      auto approving_account_objects = _remote_db->get_accounts( v_approving_account_names );

      /// TODO: recursively check one layer deeper in the authority tree for keys

      FC_ASSERT( approving_account_objects.size() == v_approving_account_names.size(), "", ("aco.size:", approving_account_objects.size())("acn",v_approving_account_names.size()) );

      flat_map< string, account_object > approving_account_lut;
      size_t i = 0;
      for( const optional< account_object >& approving_acct : approving_account_objects )
      {
         if( !approving_acct.valid() )
         {
            wlog( "operation_get_required_auths said approval of non-existing account ${name} was needed",
                  ("name", v_approving_account_names[i]) );
            i++;
            continue;
         }
         approving_account_lut[ approving_acct->name ] =  *approving_acct;
         i++;
      }
      auto get_account_from_lut = [&]( const std::string& name ) -> const account_object&
      {
         auto it = approving_account_lut.find( name );
         FC_ASSERT( it != approving_account_lut.end() );
         return it->second;
      };

      flat_set<public_key_type> approving_key_set;
      for( string& acct_name : req_active_approvals )
      {
         const auto it = approving_account_lut.find( acct_name );
         if( it == approving_account_lut.end() )
            continue;
         const account_object& acct = it->second;
         vector<public_key_type> v_approving_keys = acct.active.get_keys();
         for( const public_key_type& approving_key : v_approving_keys )
            approving_key_set.insert( approving_key );
      }

      for( string& acct_name : req_basic_approvals )
      {
         const auto it = approving_account_lut.find( acct_name );
         if( it == approving_account_lut.end() )
            continue;
         const account_object& acct = it->second;
         vector<public_key_type> v_approving_keys = acct.basic.get_keys();
         for( const public_key_type& approving_key : v_approving_keys )
            approving_key_set.insert( approving_key );
      }

      for( const string& acct_name : req_owner_approvals )
      {
         const auto it = approving_account_lut.find( acct_name );
         if( it == approving_account_lut.end() )
            continue;
         const account_object& acct = it->second;
         vector<public_key_type> v_approving_keys = acct.owner.get_keys();
         for( const public_key_type& approving_key : v_approving_keys )
            approving_key_set.insert( approving_key );
      }
      for( const authority& a : other_auths )
      {
         for( const auto& k : a.key_auths )
            approving_key_set.insert( k.first );
      }

      auto dyn_props = _remote_db->get_dynamic_global_properties();
      tx.set_reference_block( dyn_props.head_block_id );
      tx.set_expiration( dyn_props.time + fc::seconds(_tx_expiration_seconds) );
      tx.signatures.clear();

      flat_set< public_key_type > available_keys;
      flat_map< public_key_type, fc::ecc::private_key > available_private_keys;
      for( const public_key_type& key : approving_key_set )
      {
         auto it = _keys.find(key);
         if( it != _keys.end() )
         {
            fc::optional<fc::ecc::private_key> privkey = wif_to_key( it->second );
            FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
            available_keys.insert(key);
            available_private_keys[key] = *privkey;
         }
      }

      const auto keys_to_use = tx.minimize_required_signatures(
         MUSE_CHAIN_ID,
         available_keys,
         [&]( const string& account_name ) -> const authority*
         { return &(get_account_from_lut( account_name ).active); },
         [&]( const string& account_name ) -> const authority*
         { return &(get_account_from_lut( account_name ).owner); },
         [&]( const string& account_name ) -> const authority*
         { return &(get_account_from_lut( account_name ).basic); },
         [&]( const string& content_url ) -> const authority*
         {
              optional<content_object> content = *_remote_db->get_content_by_url ( content_url );
              return &content->manage_master;
         },
         [&]( const string& content_url ) -> const authority*
         {
              optional<content_object> content = *_remote_db->get_content_by_url ( content_url );
              return &content->manage_comp;
         },
         _remote_db->get_hardfork_version() < MUSE_HARDFORK_0_3_VERSION ? 1 : 2,
         MUSE_MAX_SIG_CHECK_DEPTH
         );

      for( const public_key_type& k : keys_to_use )
      {
         auto it = available_private_keys.find(k);
         FC_ASSERT( it != available_private_keys.end() );
         tx.sign( it->second, MUSE_CHAIN_ID );
      }

      if( broadcast ) {
         try {
            auto result = _remote_net_broadcast->broadcast_transaction_synchronous( tx );
            annotated_signed_transaction rtrx(tx);
            rtrx.block_num = result.get_object()["block_num"].as_uint64();
            rtrx.transaction_num = result.get_object()["trx_num"].as_uint64();
            return rtrx;
         }
         catch (const fc::exception& e)
         {
            elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()) );
            throw;
         }
      }
      return tx;
   }




   std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const
   {
      std::map<string,std::function<string(fc::variant,const fc::variants&)> > m;
      m["help"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      m["gethelp"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      m["list_my_accounts"] = [](variant result, const fc::variants& a ) {
         std::stringstream out;

         auto accounts = result.as<vector<account_object>>( GRAPHENE_MAX_NESTED_OBJECTS );
         asset total_muse;
         asset total_vest(0, VESTS_SYMBOL );
         asset total_mbd(0, MBD_SYMBOL );
         for( const auto& a : accounts ) {
            total_muse += a.balance;
            total_vest  += a.vesting_shares;
            total_mbd  += a.mbd_balance;
            out << std::left << std::setw( 17 ) << a.name
                << std::right << std::setw(20) << fc::variant( a.balance, 2 ).as_string() <<" "
                << std::right << std::setw(20) << fc::variant( a.vesting_shares, 2 ).as_string() <<" "
                << std::right << std::setw(20) << fc::variant( a.mbd_balance, 2 ).as_string() <<"\n";
         }
         out << "-------------------------------------------------------------------------\n";
            out << std::left << std::setw( 17 ) << "TOTAL"
                << std::right << std::setw(20) << fc::variant( total_muse, 2 ).as_string() <<" "
                << std::right << std::setw(20) << fc::variant( total_vest, 2 ).as_string() <<" "
                << std::right << std::setw(20) << fc::variant( total_mbd, 2 ).as_string() <<"\n";
         return out.str();
      };
      m["get_account_history"] = []( variant result, const fc::variants& a ) {
         std::stringstream ss;
         ss << std::left << std::setw( 5 )  << "#" << " ";
         ss << std::left << std::setw( 10 ) << "BLOCK #" << " ";
         ss << std::left << std::setw( 15 ) << "TRX ID" << " ";
         ss << std::left << std::setw( 20 ) << "OPERATION" << " ";
         ss << std::left << std::setw( 50 ) << "DETAILS" << "\n";
         ss << "-------------------------------------------------------------------------------\n";
         const auto& results = result.get_array();
         for( const auto& item : results ) {
            ss << std::left << std::setw(5) << item.get_array()[0].as_string() << " ";
            const auto& op = item.get_array()[1].get_object();
            ss << std::left << std::setw(10) << op["block"].as_string() << " ";
            ss << std::left << std::setw(15) << op["trx_id"].as_string() << " ";
            const auto& opop = op["op"].get_array();
            ss << std::left << std::setw(20) << opop[0].as_string() << " ";
            ss << std::left << std::setw(50) << fc::json::to_string(opop[1]) << "\n ";
         }
         return ss.str();
      };
      m["get_open_orders"] = [&]( variant result, const fc::variants& a ) {
          auto orders = result.as<vector<extended_limit_order>>( GRAPHENE_MAX_NESTED_OBJECTS );

          std::stringstream ss;

          
          ss << setiosflags( ios::fixed ) << setiosflags( ios::left ) ;
          ss << ' ' << setw( 10 ) << "Order #";
          ss << ' ' << setw( 10 ) << "Asset";
          ss << ' ' << setw( 10 ) << "Price";
          ss << ' ' << setw( 10 ) << "Quantity";
          ss << ' ' << setw( 10 ) << "Type";
          ss << "\n=====================================================================================================\n";
          for( const auto& o : orders ) {
             share_type for_sale; 
             if (o.sell_price.base.asset_id == MBD_SYMBOL )
             {
                for_sale = o.for_sale * o.sell_price.quote.amount / o.sell_price.base.amount;
             }else {
                for_sale = o.for_sale;
             }
             ss << ' ' << setw( 10 ) << o.orderid;
             ss << ' ' << setw( 10 ) << get_asset_symbol(o.sell_price.base.asset_id == MBD_SYMBOL ? o.sell_price.quote.asset_id : o.sell_price.base.asset_id );
             ss << ' ' << setw( 10 ) << o.real_price;
             ss << ' ' << setw( 10 ) << fc::variant( for_sale, 1 ).as_string();
             ss << ' ' << setw( 10 ) << (o.sell_price.base.asset_id == MBD_SYMBOL ? "BUY" : "SELL");
             ss << "\n";
          }
          return ss.str();
      };
      m["get_order_book"] = []( variant result, const fc::variants& a ) {
         auto orders = result.as< order_book >( GRAPHENE_MAX_NESTED_OBJECTS );
         std::stringstream ss;
         //asset bid_sum = asset( 0, MBD_SYMBOL );
         //asset ask_sum = asset( 0, MBD_SYMBOL );
         int spacing = 24;

         ss << setiosflags( ios::fixed ) << setiosflags( ios::left ) ;

         ss << ' ' << setw( ( spacing * 4 ) + 6 ) << "Bids" << "Asks\n"
            << ' '
            << setw( spacing + 3 ) << "Sum(" << orders.base << ")"
            << setw( spacing + 1) << orders.base
            << setw( spacing + 1 ) << orders.quote
            << setw( spacing + 1 ) << "Price"
            << setw( spacing + 1 ) << "Price"
            << setw( spacing + 1 ) << orders.quote
            << setw( spacing + 1 ) << orders.base << " " << "Sum(" << orders.base << ")"
            << "\n====================================================================================================="
            << "|=====================================================================================================\n";

         share_type bid_sum=0;
         share_type ask_sum=0;
         for( size_t i = 0; i < orders.bids.size() || i < orders.asks.size(); i++ )
         {
         
            if ( i < orders.bids.size() )
            {
               bid_sum += orders.bids[i].quote;
               ss
                  << ' ' << setw( spacing ) << bid_sum.value
                  << ' ' << setw( spacing ) << asset( orders.bids[i].quote, orders.bids[i].order_price.quote.asset_id ).to_string() << orders.base
                  << ' ' << setw( spacing ) << asset( orders.bids[i].base, MBD_SYMBOL ).to_string() << orders.quote
                  << ' ' << setw( spacing ) << orders.bids[i].real_price; //(~orders.bids[i].order_price).to_real();
            }
            else
            {
               ss << setw( (spacing * 4 ) + 5 ) << ' ';
            }

            ss << " |";

            if ( i < orders.asks.size() )
            {
               ask_sum += orders.asks[i].base;
               //ss << ' ' << setw( spacing ) << (~orders.asks[i].order_price).to_real()
               ss << ' ' << setw( spacing ) << orders.asks[i].real_price
                  << ' ' << setw( spacing ) << asset( orders.asks[i].base, MBD_SYMBOL ).to_string() << orders.quote
                  << ' ' << setw( spacing ) << asset( orders.asks[i].quote, orders.asks[i].order_price.quote.asset_id ).to_string() << orders.base
                  << ' ' << setw( spacing ) << ask_sum.value;
            }

            ss << endl;
         }

         ss << endl
            << "Bid Total: " << bid_sum.value << orders.base << endl
            << "Ask Total: " << ask_sum.value << orders.base << endl;

         return ss.str();
      };

      return m;
   }

   void use_network_node_api()
   {
      if( _remote_net_node )
         return;
      try
      {
         _remote_net_node = _remote_api->get_api_by_name("network_node_api")->as< network_node_api >();
      }
      catch( const fc::exception& e )
      {
         elog( "Couldn't get network node API" );
         throw;
      }
   }

   void use_remote_message_api()
   {
      if( _remote_message_api.valid() )
         return;

      try {
         _remote_message_api = _remote_api->get_api_by_name("private_message_api")->as< private_message_api >();
      }
      catch( const fc::exception& e )
      {
         elog( "Couldn't get private message API" );
         throw;
      }
   }

   void network_add_nodes( const vector<string>& nodes )
   {
      use_network_node_api();
      for( const string& node_address : nodes )
      {
         (*_remote_net_node)->add_node( fc::ip::endpoint::from_string( node_address ) );
      }
   }

   vector< variant > network_get_connected_peers()
   {
      use_network_node_api();
      const auto peers = (*_remote_net_node)->get_connected_peers();
      vector< variant > result;
      result.reserve( peers.size() );
      for( const auto& peer : peers )
      {
         variant v;
         fc::to_variant( peer, v, GRAPHENE_MAX_NESTED_OBJECTS );
         result.push_back( v );
      }
      return result;
   }

   operation get_prototype_operation( string operation_name )
   {
      auto it = _prototype_ops.find( operation_name );
      if( it == _prototype_ops.end() )
         FC_THROW("Unsupported operation: \"${operation_name}\"", ("operation_name", operation_name));
      return it->second;
   }

   string get_asset_symbol(asset_id_type asset_id)const
   {
      return _remote_db->get_asset(asset_id).symbol_string;
   }

   optional<asset_object> find_asset(string asset_symbol)const
   {
      auto rec = _remote_db->get_uia_details(asset_symbol);
      if( rec )
      {
         if( rec->symbol_string != asset_symbol )
            return optional<asset_object>();
      }
      return rec;
   }

   vector<asset_object> list_assets( uint32_t start_id, uint32_t limit)
   {
      return _remote_db->lookup_uias(start_id);
   }
   
   vector <account_balance_object> get_assets (string account)
   {
      return _remote_db->get_uia_balances( account );  
   }

   annotated_signed_transaction create_asset(string issuer, string asset_name, string description, uint64_t max_supply, bool broadcast)
   {
      try {
         account_object issuer_account = get_account( issuer );
         FC_ASSERT(!find_asset(asset_name).valid(), "Asset with that symbol already exists!");
         asset_create_operation create_op;
         create_op.issuer = issuer_account.name;
         create_op.symbol = asset_name;
         create_op.common_options.max_supply=max_supply;
         create_op.common_options.flags=override_authority|disable_confidential;
         signed_transaction tx;
         tx.operations.push_back( create_op );
         tx.validate();
         return sign_transaction( tx, broadcast );
      }FC_CAPTURE_AND_RETHROW( (issuer)(asset_name)(description)(max_supply) ) 
   }


   annotated_signed_transaction update_asset(string asset_name, string description, uint64_t max_supply, string new_issuer, bool broadcast)
   {
      try {
         optional<asset_object> asset_to_update = find_asset(asset_name);
         if (!asset_to_update)
            FC_THROW("No asset with that symbol exists!");
         optional<account_id_type> new_issuer_account_id;
         
         asset_update_operation update_op;
         update_op.issuer = get_account_from_id(asset_to_update->issuer)->name;
         update_op.new_options.flags=override_authority|disable_confidential;
         update_op.asset_to_update = asset_to_update->id;
         if(new_issuer.length()>0){
            get_account( new_issuer );
            update_op.new_issuer = new_issuer;
         }
         update_op.new_options.max_supply=max_supply;
         if(description.length()>0){
            update_op.new_options.description=description;
         }

         signed_transaction tx;
         tx.operations.push_back( update_op );
         tx.validate();
         return sign_transaction( tx, broadcast );
      }FC_CAPTURE_AND_RETHROW( (asset_name)(description)(max_supply)(new_issuer)(broadcast) )
   }
   
   annotated_signed_transaction issue_asset(string asset_name, string to_account, fc::real128 amount, bool broadcast)
   {
      auto asset_obj = find_asset(asset_name);
      auto to_account_obj = get_account(to_account);
      if (!asset_obj)
         FC_THROW("No asset with that symbol exists!");
      auto issuer = get_account_from_id(asset_obj->issuer);
      asset_issue_operation issue_op;
      issue_op.issuer = issuer->name;
      uint64_t amount_i = (amount*asset::static_precision()).to_uint64();
      issue_op.asset_to_issue = asset(amount_i, asset_obj->id);
      issue_op.issue_to_account=to_account;

      signed_transaction tx;
      tx.operations.push_back(issue_op);
      tx.validate();

      return sign_transaction(tx, broadcast);
   }

   annotated_signed_transaction reserve_asset(string asset_name, string from_account, fc::real128 amount, bool broadcast)
   {
      auto asset_obj = find_asset(asset_name);
      auto from_account_obj = get_account(from_account);
      if (!asset_obj)
         FC_THROW("No asset with that symbol exists!");
      auto issuer = get_account_from_id(asset_obj->issuer);
      
      asset_reserve_operation reserve_op;
      reserve_op.issuer = issuer->name;

      uint64_t amount_i = (amount*asset::static_precision()).to_uint64();

      reserve_op.amount_to_reserve = asset(amount_i, asset_obj->id);
      reserve_op.payer = from_account;

      signed_transaction tx;
      tx.operations.push_back( reserve_op );
      tx.validate();

      return sign_transaction( tx, broadcast );

   }

   string                                  _wallet_filename;
   wallet_data                             _wallet;

   map<public_key_type,string>             _keys;
   fc::sha512                              _checksum;
   fc::api<login_api>                      _remote_api;
   fc::api<database_api>                   _remote_db;
   fc::api<network_broadcast_api>          _remote_net_broadcast;
   optional< fc::api<network_node_api> >   _remote_net_node;
   optional< fc::api<private_message_api> > _remote_message_api;
   uint32_t                                _tx_expiration_seconds = 30;

   flat_map<string, operation>             _prototype_ops;

   static_variant_map _operation_which_map = create_static_variant_map< operation >();

#ifdef __unix__
   mode_t                  _old_umask;
#endif
   const string _wallet_filename_extension = ".wallet";
};

} } } // muse::wallet::detail



namespace muse { namespace wallet {

wallet_api::wallet_api(const wallet_data& initial_data, fc::api<login_api> rapi)
   : my(new detail::wallet_api_impl(*this, initial_data, rapi))
{}

wallet_api::~wallet_api(){}

bool wallet_api::copy_wallet_file(string destination_filename)
{
   return my->copy_wallet_file(destination_filename);
}

optional<signed_block_with_info> wallet_api::get_block(uint32_t num)
{
   return my->_remote_db->get_block(num);
}

vector<account_object> wallet_api::list_my_accounts()
{
   FC_ASSERT( !is_locked(), "Wallet must be unlocked to list accounts" );
   vector<public_key_type> pub_keys;
   pub_keys.reserve( my->_keys.size() );

   for( const auto& item : my->_keys )
      pub_keys.push_back(item.first);

   auto refs = my->_remote_db->get_key_references( pub_keys );
   set<string> names;
   for( const auto& item : refs )
      for( const auto& name : item )
         names.insert( name );

   vector<account_object> result;
   result.reserve( names.size() );
   for( const auto& name : names )
      result.emplace_back( get_account( name ) );

   return result;
}

set<string> wallet_api::list_accounts(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_accounts(lowerbound, limit);
}

vector<string> wallet_api::get_active_witnesses()const {
   return my->_remote_db->get_active_witnesses();
}

vector<string> wallet_api::get_voted_streaming_platforms()const
{
   return my->_remote_db->get_voted_streaming_platforms();
}

brain_key_info wallet_api::suggest_brain_key()const
{
   brain_key_info result;
   // create a private key for secure entropy
   fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
   fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
   fc::bigint entropy1( sha_entropy1.data(), sha_entropy1.data_size() );
   fc::bigint entropy2( sha_entropy2.data(), sha_entropy2.data_size() );
   fc::bigint entropy(entropy1);
   entropy <<= 8*sha_entropy1.data_size();
   entropy += entropy2;
   string brain_key = "";

   for( int i=0; i<BRAIN_KEY_WORD_COUNT; i++ )
   {
      fc::bigint choice = entropy % graphene::words::word_list_size;
      entropy /= graphene::words::word_list_size;
      if( i > 0 )
         brain_key += " ";
      brain_key += graphene::words::word_list[ choice.to_int64() ];
   }

   brain_key = normalize_brain_key(brain_key);
   fc::ecc::private_key priv_key = detail::derive_private_key( brain_key, 0 );
   result.brain_priv_key = brain_key;
   result.wif_priv_key = key_to_wif( priv_key );
   result.pub_key = priv_key.get_public_key();
   return result;
}

string wallet_api::serialize_transaction( signed_transaction tx )const
{
   return fc::to_hex(fc::raw::pack_to_vector(tx));
}

string wallet_api::get_wallet_filename() const
{
   return my->get_wallet_filename();
}


extended_account wallet_api::get_account( string account_name ) const
{
   return my->get_account( account_name );
}

bool wallet_api::import_key(string wif_key)
{
   FC_ASSERT(!is_locked());
   // backup wallet
   fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
   if (!optional_private_key)
      FC_THROW("Invalid private key");
//   string shorthash = detail::pubkey_to_shorthash( optional_private_key->get_public_key() );
//   copy_wallet_file( "before-import-key-" + shorthash );

   if( my->import_key(wif_key) )
   {
      save_wallet_file();
 //     copy_wallet_file( "after-import-key-" + shorthash );
      return true;
   }
   return false;
}

string wallet_api::normalize_brain_key(string s) const
{
   return detail::normalize_brain_key( s );
}

variant wallet_api::info()
{
   return my->info();
}

variant_object wallet_api::about() const
{
    return my->about();
}

/*
fc::ecc::private_key wallet_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
{
   return detail::derive_private_key( prefix_string, sequence_number );
}
*/

set<string> wallet_api::list_witnesses(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_witness_accounts(lowerbound, limit);
}

set<string> wallet_api::list_streaming_platforms(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_streaming_platform_accounts(lowerbound, limit);
}

optional< witness_object > wallet_api::get_witness(string owner_account)
{
   return my->get_witness(owner_account);
}

annotated_signed_transaction wallet_api::set_voting_proxy(string account_to_modify, string voting_account, bool broadcast /* = false */)
{ return my->set_voting_proxy(account_to_modify, voting_account, broadcast); }

void wallet_api::set_wallet_filename(string wallet_filename) { my->_wallet_filename = wallet_filename; }

annotated_signed_transaction wallet_api::sign_transaction(signed_transaction tx, bool broadcast /* = false */)
{ try {
   return my->sign_transaction( tx, broadcast);
} FC_CAPTURE_AND_RETHROW( (tx) ) }

operation wallet_api::get_prototype_operation(string operation_name) {
   return my->get_prototype_operation( operation_name );
}

void wallet_api::network_add_nodes( const vector<string>& nodes ) {
   my->network_add_nodes( nodes );
}

vector< variant > wallet_api::network_get_connected_peers() {
   return my->network_get_connected_peers();
}

string wallet_api::help()const
{
   std::vector<std::string> method_names = my->method_documentation.get_method_names();
   std::stringstream ss;
   for (const std::string method_name : method_names)
   {
      try
      {
         ss << my->method_documentation.get_brief_description(method_name);
      }
      catch (const fc::key_not_found_exception&)
      {
         ss << method_name << " (no help available)\n";
      }
   }
   return ss.str();
}

string wallet_api::gethelp(const string& method)const
{
   fc::api<wallet_api> tmp;
   std::stringstream ss;
   ss << "\n";

   std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
   if (!doxygenHelpString.empty())
      ss << doxygenHelpString;
   else
      ss << "No help defined for method " << method << "\n";

   return ss.str();
}

bool wallet_api::load_wallet_file( string wallet_filename )
{
   return my->load_wallet_file( wallet_filename );
}

void wallet_api::save_wallet_file( string wallet_filename )
{
   my->save_wallet_file( wallet_filename );
}

std::map<string,std::function<string(fc::variant,const fc::variants&)> >
wallet_api::get_result_formatters() const
{
   return my->get_result_formatters();
}

bool wallet_api::is_locked()const
{
   return my->is_locked();
}
bool wallet_api::is_new()const
{
   return my->_wallet.cipher_keys.size() == 0;
}

void wallet_api::encrypt_keys()
{
   my->encrypt_keys();
}

void wallet_api::lock()
{ try {
   FC_ASSERT( !is_locked() );
   encrypt_keys();
   for( auto key : my->_keys )
      key.second = key_to_wif(fc::ecc::private_key());
   my->_keys.clear();
   my->_checksum = fc::sha512();
   my->self.lock_changed(true);
} FC_CAPTURE_AND_RETHROW() }

void wallet_api::unlock(string password)
{ try {
   FC_ASSERT(password.size() > 0);
   auto pw = fc::sha512::hash(password.c_str(), password.size());
   vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
   auto pk = fc::raw::unpack_from_vector<plain_keys>(decrypted);
   FC_ASSERT(pk.checksum == pw);
   my->_keys = std::move(pk.keys);
   my->_checksum = pk.checksum;
   my->self.lock_changed(false);
} FC_CAPTURE_AND_RETHROW() }

void wallet_api::set_password( string password )
{
   if( !is_new() )
      FC_ASSERT( !is_locked(), "The wallet must be unlocked before the password can be set" );
   my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
   lock();
}

vector<proposal_object> wallet_api::get_proposed_transactions( string id )const
{
   return my->get_proposed_transactions( id );
}

map<public_key_type, string> wallet_api::list_keys()
{
   FC_ASSERT(!is_locked());
   return my->_keys;
}

string wallet_api::get_private_key( public_key_type pubkey )const
{
   return key_to_wif( my->get_private_key( pubkey ) );
}

pair<public_key_type,string> wallet_api::get_private_key_from_password( string account, string role, string password )const {
   auto seed = account + role + password;
   FC_ASSERT( seed.size() );
   auto secret = fc::sha256::hash( seed.c_str(), seed.size() );
   auto priv = fc::ecc::private_key::regenerate( secret );
   return std::make_pair( public_key_type( priv.get_public_key() ), key_to_wif( priv ) );
}

signed_block_with_info::signed_block_with_info( const signed_block& block )
   : signed_block( block )
{
   block_id = id();
   signing_key = signee();
   transaction_ids.reserve( transactions.size() );
   for( const signed_transaction& tx : transactions )
      transaction_ids.push_back( tx.id() );
}

feed_history_object wallet_api::get_feed_history()const { return my->_remote_db->get_feed_history(); }

/**
 * This method is used by faucets to create new accounts for other users which must
 * provide their desired keys. The resulting account may not be controllable by this
 * wallet.
 */
annotated_signed_transaction wallet_api::create_account_with_keys( string creator,
                                      string new_account_name,
                                      string json_meta,
                                      public_key_type owner,
                                      public_key_type active,
                                      public_key_type basic,
                                      public_key_type memo,
                                      bool broadcast )const
{ try {
   FC_ASSERT( !is_locked() );
   account_create_operation op;
   op.creator = creator;
   op.new_account_name = new_account_name;
   op.owner = authority( 1, owner, 1 );
   op.active = authority( 1, active, 1 );
   op.basic = authority( 1, basic, 1 );
   op.memo_key = memo;
   op.json_metadata = json_meta;
   op.fee = my->_remote_db->get_chain_properties().account_creation_fee;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (creator)(new_account_name)(json_meta)(owner)(active)(memo)(broadcast) ) }

annotated_signed_transaction wallet_api::request_account_recovery( string recovery_account, string account_to_recover, authority new_authority, bool broadcast )
{
   FC_ASSERT( !is_locked() );
   request_account_recovery_operation op;
   op.recovery_account = recovery_account;
   op.account_to_recover = account_to_recover;
   op.new_owner_authority = new_authority;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::recover_account( string account_to_recover, authority recent_authority, authority new_authority, bool broadcast ) {
   FC_ASSERT( !is_locked() );

   recover_account_operation op;
   op.account_to_recover = account_to_recover;
   op.new_owner_authority = new_authority;
   op.recent_owner_authority = recent_authority;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::change_recovery_account( string owner, string new_recovery_account, bool broadcast ) {
   FC_ASSERT( !is_locked() );

   change_recovery_account_operation op;
   op.account_to_recover = owner;
   op.new_recovery_account = new_recovery_account;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

vector< owner_authority_history_object > wallet_api::get_owner_history( string account )const
{
   return my->_remote_db->get_owner_history( account );
}

annotated_signed_transaction wallet_api::update_account(
                                      string account_name,
                                      string json_meta,
                                      public_key_type owner,
                                      public_key_type active,
                                      public_key_type basic,
                                      public_key_type memo,
                                      bool broadcast )const
{
   try
   {
      FC_ASSERT( !is_locked() );

      account_update_operation op;
      op.account = account_name;
      op.owner  = authority( 1, owner, 1 );
      op.active = authority( 1, active, 1);
      op.basic = authority( 1, basic, 1);
      op.memo_key = memo;
      op.json_metadata = json_meta;

      signed_transaction tx;
      tx.operations.push_back(op);
      tx.validate();

      return my->sign_transaction( tx, broadcast );
   }
   FC_CAPTURE_AND_RETHROW( (account_name)(json_meta)(owner)(active)(memo)(broadcast) )
}

annotated_signed_transaction wallet_api::update_account_auth_key( string account_name, authority_type type, public_key_type key, weight_type weight, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   auto accounts = my->_remote_db->get_accounts( { account_name } );
   FC_ASSERT( accounts.size() == 1, "Account does not exist" );
   FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );

   account_update_operation op;
   op.account = account_name;
   op.memo_key = accounts[0].memo_key;
   op.json_metadata = accounts[0].json_metadata;

   authority new_auth;

   switch( type )
   {
      case( owner ):
         new_auth = accounts[0].owner;
         break;
      case( active ):
         new_auth = accounts[0].active;
         break;
      case( basic ):
         new_auth = accounts[0].basic;
         break;
   }

   if( weight == 0 ) // Remove the key
   {
      new_auth.key_auths.erase( key );
   }
   else
   {
      new_auth.add_authority( key, weight );
   }

   if( new_auth.is_impossible() )
   {
      if ( type == owner )
      {
         FC_ASSERT( false, "Owner authority change would render account irrecoverable." );
      }

      wlog( "Authority is now impossible." );
   }

   switch( type )
   {
      case( owner ):
         op.owner = new_auth;
         break;
      case( active ):
         op.active = new_auth;
         break;
      case( basic ):
         op.basic = new_auth;
         break;
   }

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_account_auth_account( string account_name, authority_type type, string auth_account, weight_type weight, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   auto accounts = my->_remote_db->get_accounts( { account_name } );
   FC_ASSERT( accounts.size() == 1, "Account does not exist" );
   FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );

   account_update_operation op;
   op.account = account_name;
   op.memo_key = accounts[0].memo_key;
   op.json_metadata = accounts[0].json_metadata;

   authority new_auth;

   switch( type )
   {
      case( owner ):
         new_auth = accounts[0].owner;
         break;
      case( active ):
         new_auth = accounts[0].active;
         break;
      case( basic ):
         new_auth = accounts[0].basic;
         break;
   }

   if( weight == 0 ) // Remove the key
   {
      new_auth.account_auths.erase( auth_account );
   }
   else
   {
      new_auth.add_authority( auth_account, weight );
   }

   if( new_auth.is_impossible() )
   {
      if ( type == owner )
      {
         FC_ASSERT( false, "Owner authority change would render account irrecoverable." );
      }

      wlog( "Authority is now impossible." );
   }

   switch( type )
   {
      case( owner ):
         op.owner = new_auth;
         break;
      case( active ):
         op.active = new_auth;
         break;
      case( basic ):
         op.basic = new_auth;
         break;
   }

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_account_auth_threshold( string account_name, authority_type type, uint32_t threshold, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   auto accounts = my->_remote_db->get_accounts( { account_name } );
   FC_ASSERT( accounts.size() == 1, "Account does not exist" );
   FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );
   FC_ASSERT( threshold != 0, "Authority is implicitly satisfied" );

   account_update_operation op;
   op.account = account_name;
   op.memo_key = accounts[0].memo_key;
   op.json_metadata = accounts[0].json_metadata;

   authority new_auth;

   switch( type )
   {
      case( owner ):
         new_auth = accounts[0].owner;
         break;
      case( active ):
         new_auth = accounts[0].active;
         break;
      case( basic ):
         new_auth = accounts[0].basic;
         break;
   }

   new_auth.weight_threshold = threshold;

   if( new_auth.is_impossible() )
   {
      if ( type == owner )
      {
         FC_ASSERT( false, "Owner authority change would render account irrecoverable." );
      }

      wlog( "Authority is now impossible." );
   }

   switch( type )
   {
      case( owner ):
         op.owner = new_auth;
         break;
      case( active ):
         op.active = new_auth;
         break;
      case( basic ):
         op.basic = new_auth;
         break;
   }

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_account_meta( string account_name, string json_meta, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   auto accounts = my->_remote_db->get_accounts( { account_name } );
   FC_ASSERT( accounts.size() == 1, "Account does not exist" );
   FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );

   account_update_operation op;
   op.account = account_name;
   op.memo_key = accounts[0].memo_key;
   op.json_metadata = json_meta;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_account_memo_key( string account_name, public_key_type key, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   auto accounts = my->_remote_db->get_accounts( { account_name } );
   FC_ASSERT( accounts.size() == 1, "Account does not exist" );
   FC_ASSERT( account_name == accounts[0].name, "Account name doesn't match?" );

   account_update_operation op;
   op.account = account_name;
   op.memo_key = key;
   op.json_metadata = accounts[0].json_metadata;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

/**
 *  This method will genrate new owner, active, and memo keys for the new account which
 *  will be controlable by this wallet.
 */
annotated_signed_transaction wallet_api::create_account( string creator, string new_account_name, string json_meta, bool broadcast )
{ try {
   FC_ASSERT( !is_locked() );
   auto owner = suggest_brain_key();
   auto active = suggest_brain_key();
   auto basic = suggest_brain_key();
   auto memo = suggest_brain_key();
   import_key( owner.wif_priv_key );
   import_key( active.wif_priv_key );
   import_key( basic.wif_priv_key );
   import_key( memo.wif_priv_key );
   return create_account_with_keys( creator, new_account_name, json_meta, owner.pub_key, active.pub_key, basic.pub_key, memo.pub_key, broadcast );
} FC_CAPTURE_AND_RETHROW( (creator)(new_account_name)(json_meta) ) }

annotated_signed_transaction wallet_api::friendship( string who, string whom, bool broadcast )
{
   FC_ASSERT( !is_locked() );
   friendship_operation op;
   op.who = who;
   op.whom = whom;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_witness( string witness_account_name,
                                               string url,
                                               public_key_type block_signing_key,
                                               const chain_properties& props,
                                               bool broadcast  )
{
   FC_ASSERT( !is_locked() );
   witness_update_operation op;
   op.owner = witness_account_name;
   op.url = url;
   op.block_signing_key = block_signing_key;
   op.props = props;

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_streaming_platform( string streaming_platform_name, string url, bool broadcast )
{
   FC_ASSERT( !is_locked() );
   streaming_platform_update_operation op;
   op.owner = streaming_platform_name;
   op.url=url;
   op.fee = asset(MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE, MUSE_SYMBOL);

   signed_transaction tx;
   tx.operations.push_back(op);
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::vote_for_witness(string voting_account, string witness_to_vote_for, bool approve, bool broadcast )
{ try {
   FC_ASSERT( !is_locked() );
    account_witness_vote_operation op;
    op.account = voting_account;
    op.witness = witness_to_vote_for;
    op.approve = approve;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (voting_account)(witness_to_vote_for)(approve)(broadcast) ) }

annotated_signed_transaction wallet_api::vote_for_streaming_platform(string voting_account, string streaming_platform_to_vote_for, bool approve, bool broadcast )
{ try {
   FC_ASSERT( !is_locked() );
    account_streaming_platform_vote_operation op;
    op.account = voting_account;
    op.streaming_platform = streaming_platform_to_vote_for;
    op.approve = approve;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (voting_account)(streaming_platform_to_vote_for)(approve)(broadcast) ) }

annotated_signed_transaction wallet_api::transfer(string from, string to, asset amount, string memo, bool broadcast )
{ try {
   FC_ASSERT( !is_locked() );
    transfer_operation op;
    op.from = from;
    op.to = to;
    op.amount = amount;

    if( memo.size() > 0 && memo[0] == '#' ) {
       memo_data m;

       auto from_account = get_account( from );
       auto to_account   = get_account( to );

       m.from            = from_account.memo_key;
       m.to              = to_account.memo_key;
       m.nonce = fc::time_point::now().time_since_epoch().count();

       auto from_priv = my->get_private_key( m.from );
       auto shared_secret = from_priv.get_shared_secret( m.to );

       fc::sha512::encoder enc;
       fc::raw::pack( enc, m.nonce );
       fc::raw::pack( enc, shared_secret );
       auto encrypt_key = enc.result();

       m.encrypted = fc::aes_encrypt( encrypt_key, fc::raw::pack_to_vector(memo.substr(1)) );
       m.check = fc::sha256::hash( encrypt_key )._hash[0];
       op.memo = m;
    } else {
       op.memo = memo;
    }

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(memo)(broadcast) ) }

annotated_signed_transaction wallet_api::transfer_to_vesting(string from, string to, asset amount, bool broadcast )
{
   FC_ASSERT( !is_locked() );
    transfer_to_vesting_operation op;
    op.from = from;
    op.to = (to == from ? "" : to);
    op.amount = amount;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
}
annotated_signed_transaction wallet_api::withdraw_vesting(string from, asset vesting_shares, bool broadcast )
{
   FC_ASSERT( !is_locked() );
    withdraw_vesting_operation op;
    op.account = from;
    op.vesting_shares = vesting_shares;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::set_withdraw_vesting_route( string from, string to, uint16_t percent, bool auto_vest, bool broadcast )
{
   FC_ASSERT( !is_locked() );
    set_withdraw_vesting_route_operation op;
    op.from_account = from;
    op.to_account = to;
    op.percent = percent;
    op.auto_vest = auto_vest;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::convert_mbd(string from, asset amount, bool broadcast )
{
   FC_ASSERT( !is_locked() );
    convert_operation op;
    op.owner = from;
    op.requestid = fc::time_point::now().sec_since_epoch();
    op.amount = amount;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::publish_feed(string witness, price exchange_rate, bool broadcast )
{
   FC_ASSERT( !is_locked() );
    feed_publish_operation op;
    op.publisher     = witness;
    op.exchange_rate = exchange_rate;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

   return my->sign_transaction( tx, broadcast );
}

vector< convert_request_object > wallet_api::get_conversion_requests( string owner_account )
{
   return my->_remote_db->get_conversion_requests( owner_account );
}

map<uint32_t,operation_object> wallet_api::get_account_history( string account, uint32_t from, uint32_t limit ) {
   auto result = my->_remote_db->get_account_history(account,from,limit);
   if( !is_locked() ) {
      for( auto& item : result ) {
         if( item.second.op.which() == operation::tag<transfer_operation>::value ) {
            auto& top = item.second.op.get<transfer_operation>();
            if( top.memo.size() && top.memo[0] == '#' ) {
               auto m = memo_data::from_string( top.memo );
               if( m ) {
                  fc::sha512 shared_secret;
                  auto from_key = my->try_get_private_key( m->from );
                  if( !from_key ) {
                     auto to_key   = my->try_get_private_key( m->to );
                     if( !to_key ) return result;
                     shared_secret = to_key->get_shared_secret( m->from );
                  } else {
                     shared_secret = from_key->get_shared_secret( m->to );
                  }
                  fc::sha512::encoder enc;
                  fc::raw::pack( enc, m->nonce );
                  fc::raw::pack( enc, shared_secret );
                  auto encryption_key = enc.result();

                  uint32_t check = fc::sha256::hash( encryption_key )._hash[0];
                  if( check != m->check ) return result;

                  try {
                     vector<char> decrypted = fc::aes_decrypt( encryption_key, m->encrypted );
                     top.memo = fc::raw::unpack_from_vector<std::string>( decrypted );
                  } catch ( ... ){}
               }
            }
         }
      }
   }
   return result;
}

/*app::state wallet_api::get_state( string url ) {
   return my->_remote_db->get_state(url);
}*/

order_book wallet_api::get_order_book( uint32_t limit )
{
   FC_ASSERT( limit <= 1000 );
   return my->_remote_db->get_order_book( limit );
}
vector<extended_limit_order> wallet_api::get_open_orders( string owner )
{
   return my->_remote_db->get_open_orders( owner );
}

annotated_signed_transaction wallet_api::create_order(  string owner, uint32_t order_id, asset amount_to_sell, asset min_to_receive, bool fill_or_kill, uint32_t expiration_sec, bool broadcast )
{
   FC_ASSERT( !is_locked() );
   limit_order_create_operation op;
   op.owner = owner;
   op.orderid = order_id;
   op.amount_to_sell = amount_to_sell;
   op.min_to_receive = min_to_receive;
   op.fill_or_kill = fill_or_kill;
   op.expiration = expiration_sec ? (fc::time_point::now() + fc::seconds(expiration_sec)) : fc::time_point::maximum();

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::cancel_order( string owner, uint32_t orderid, bool broadcast ) {
   FC_ASSERT( !is_locked() );
   limit_order_cancel_operation op;
   op.owner = owner;
   op.orderid = orderid;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}


vector<content_object> wallet_api::get_content_by_account(string account)
{
    return my->get_content_by_account(account);
}

vector<content_object> detail::wallet_api_impl::get_content_by_account(string account)
{
    return _remote_db->get_content_by_uploader(account);
}

optional<content_object>  wallet_api::get_content_by_url(string url)
{   
    return my->get_content_by_url(url);
}

optional<content_object>  detail::wallet_api_impl::get_content_by_url(string url)
{   
    return _remote_db->get_content_by_url(url);
}

vector<content_object>  wallet_api::lookup_content(const string& start, uint32_t limit )
{
    return my->lookup_content(start, limit);
}

vector<content_object>  detail::wallet_api_impl::lookup_content(const string& start, uint32_t limit )
{
    return _remote_db->lookup_content(start, limit);
}


annotated_signed_transaction wallet_api::import_balance( string name_or_id, const vector<string>& wif_keys, bool broadcast )
{
   return my->import_balance( name_or_id, wif_keys, broadcast );
}

vector<balance_object> wallet_api::get_balance_objects( const string& pub_key )
{
   vector< address > addrs;
   addrs.reserve( 5 );

      fc::ecc::public_key pk = fc::ecc::public_key::from_base58(pub_key);
      addrs.push_back( address(pk) );
      addrs.push_back( pts_address( pk, false, 56 ) );
      addrs.push_back( pts_address( pk, true, 56 ) );
      addrs.push_back( pts_address( pk, false, 0 ) );
      addrs.push_back( pts_address( pk, true, 0 ) );


   return my->_remote_db->get_balance_objects( addrs );
}


annotated_signed_transaction detail::wallet_api_impl::import_balance( string name_or_id, const vector<string>& wif_keys, bool broadcast )
{ try {
   FC_ASSERT(!is_locked());
   account_object claimer = get_account( name_or_id );

   map< address, private_key_type > keys;  // local index of address -> private key
   vector< address > addrs;
   addrs.reserve( wif_keys.size() * 5 );
   for( const string& wif_key : wif_keys )
   {
      optional< private_key_type > key = wif_to_key( wif_key );
      FC_ASSERT( key.valid(), "Invalid private key" );

      fc::ecc::public_key pk = key->get_public_key();
      addrs.push_back( address(pk) );
      keys[addrs.back()] = *key;
      addrs.push_back( pts_address( pk, false, 56 ) );
      keys[addrs.back()] = *key;
      addrs.push_back( pts_address( pk, true, 56 ) );
      keys[addrs.back()] = *key;
      addrs.push_back( pts_address( pk, false, 0 ) );
      keys[addrs.back()] = *key;
      addrs.push_back( pts_address( pk, true, 0 ) );
      keys[addrs.back()] = *key;
   }

   vector< balance_object > balances = _remote_db->get_balance_objects( addrs );
   addrs.clear();

   signed_transaction tx;
   set<private_key_type> signing_keys;
   for( const balance_object& b : balances ){
      balance_claim_operation op;
      op.deposit_to_account = claimer.name;
      op.balance_to_claim = b.id;
      op.balance_owner_key = keys[b.owner].get_public_key();
      signing_keys.insert(keys[b.owner]);
      op.total_claimed = b.balance;
      tx.operations.push_back( op );
   }

   tx.validate();
   auto dyn_props = _remote_db->get_dynamic_global_properties();
   tx.set_reference_block( dyn_props.head_block_id );
   tx.set_expiration( dyn_props.time + fc::seconds(_tx_expiration_seconds) );
   tx.signatures.clear();


   for( const private_key_type& pk : signing_keys )
   {
      tx.sign( pk, MUSE_CHAIN_ID );
   }

   if( broadcast ) {
      try {
            auto result = _remote_net_broadcast->broadcast_transaction_synchronous( tx );
            annotated_signed_transaction rtrx(tx);
            rtrx.block_num = result.get_object()["block_num"].as_uint64();
            rtrx.transaction_num = result.get_object()["trx_num"].as_uint64();
            return rtrx;
      }
      catch (const fc::exception& e)
      {
            elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()) );
            throw;
      }
   }
   return tx;
} FC_CAPTURE_AND_RETHROW( (name_or_id) ) }



annotated_signed_transaction wallet_api::create_content(string uploader, string url, string title, vector<distribution> distributor,
                                                        vector<management_vote> management, uint32_t threshold,
                                                        uint32_t playing_reward, uint32_t publishers_share,
                                                        bool third_party_publishers, vector<distribution> distributor_publishing,
                                                        vector<management_vote> management_publishing,
                                                        uint32_t threshold_publishing, bool broadcast) {
   FC_ASSERT( !is_locked() );
   content_operation op;
   op.uploader = uploader;
   op.url = url;
   op.track_meta.track_title = title;
   op.album_meta.album_title = title;
   op.distributions = distributor;
   op.management = management;
   op.management_threshold = threshold;
   if( playing_reward > 0 )
      op.playing_reward = playing_reward;
   op.comp_meta.third_party_publishers = third_party_publishers;
   if( third_party_publishers )
   {
      FC_ASSERT(management_publishing.size() > 0);
      op.management_comp = management_publishing;
      op.management_threshold_comp = threshold_publishing;
      if( distributor_publishing.size()>0 )
         op.distributions_comp = distributor_publishing;
   }
   op.publishers_share = publishers_share;
   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();
   
   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::approve_content( string approver, string url,  bool broadcast )
{
   FC_ASSERT( !is_locked() );
   content_approve_operation op;
   op.approver = approver;
   op.url = url;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();
   
   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_content_master(string url, string new_title, vector<distribution> new_distributor,
                                                               vector<management_vote> new_management, uint32_t new_threshold,
                                                               bool broadcast) {
   FC_ASSERT( !is_locked() );
   content_update_operation op;
   op.url = url;
   op.side = content_update_operation::side_t::master;
   if(new_title.size()) {
      content_metadata_track_master md;
      optional<content_metadata_track_master> omd( md );
      op.track_meta = omd;
      op.track_meta->track_title = new_title;
   }
   op.new_distributions = new_distributor;
   op.new_management = new_management;
   op.new_threshold = new_threshold;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_content_publishing(string url, string new_title, vector<distribution> new_distributor,
                                                                   vector<management_vote> new_management, uint32_t new_threshold, bool broadcast) {
   FC_ASSERT( !is_locked() );
   content_update_operation op;
   op.url = url;
   op.side = content_update_operation::side_t::publisher;
   if(new_title.size()) {
      content_metadata_publisher md;
      op.comp_meta = md;
      op.comp_meta->composition_title = new_title;
   }
   op.new_distributions = new_distributor;
   op.new_management = new_management;
   op.new_playing_reward = 0;
   op.new_publishers_share = 0;
   op.new_threshold = new_threshold;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::update_content_global(string url, uint32_t new_publishers_share, uint32_t new_playing_reward, bool broadcast)
{
   FC_ASSERT( !is_locked() );
   content_update_operation op;
   op.url = url;
   op.side = content_update_operation::side_t::master;
   op.new_publishers_share = new_publishers_share;
   op.new_playing_reward = new_playing_reward;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::disable_content( string url, bool broadcast )
{
   FC_ASSERT( !is_locked() );
   content_disable_operation op;
   op.url = url;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::vote(string voter, string url, int16_t weight, bool broadcast)
{
   FC_ASSERT( !is_locked() );
   FC_ASSERT( abs(weight) <= 100, "Weight must be between -100 and 100 and not 0" );

   vote_operation op;
   op.voter = voter;
   op.url = url;
   op.weight = weight * MUSE_1_PERCENT;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

void wallet_api::set_transaction_expiration(uint32_t seconds)
{
   my->set_transaction_expiration(seconds);
}

annotated_signed_transaction wallet_api::challenge( string challenger, string challenged, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   challenge_authority_operation op;
   op.challenger = challenger;
   op.challenged = challenged;
   op.require_owner = false;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

annotated_signed_transaction wallet_api::prove( string challenged, bool broadcast )
{
   FC_ASSERT( !is_locked() );

   prove_authority_operation op;
   op.challenged = challenged;
   op.require_owner = false;

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}


annotated_signed_transaction wallet_api::get_transaction( transaction_id_type id )const {
   return my->_remote_db->get_transaction( id );
}



annotated_signed_transaction      wallet_api::post_streaming_report( string streaming_platform, string consumer, string content, string playlist_creator, uint64_t playtime, bool broadcast )
{
    streaming_platform_report_operation op;
    op.streaming_platform = streaming_platform;
    op.consumer = consumer;
    op.content = content;
    op.playlist_creator = playlist_creator;
    op.play_time = playtime;
 
    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

    return my->sign_transaction( tx, broadcast );

}

annotated_signed_transaction      wallet_api::send_private_message( string from, string to, string subject, string body, bool broadcast ) {
   FC_ASSERT( !is_locked(), "wallet must be unlocked to send a private message" );
   auto from_account = get_account( from );
   auto to_account   = get_account( to );

   custom_operation op;
   op.required_auths.insert(from);
   op.id = MUSE_PRIVATE_MESSAGE_COP_ID;


   private_message_operation pmo;
   pmo.from          = from;
   pmo.to            = to;
   pmo.sent_time     = fc::time_point::now().time_since_epoch().count();
   pmo.from_memo_key = from_account.memo_key;
   pmo.to_memo_key   = to_account.memo_key;

   message_body message;
   message.subject = subject;
   message.body    = body;

   auto priv_key = wif_to_key( get_private_key( pmo.from_memo_key ) );
   FC_ASSERT( priv_key, "unable to find private key for memo" );
   auto shared_secret = priv_key->get_shared_secret( pmo.to_memo_key );
   fc::sha512::encoder enc;
   fc::raw::pack( enc, pmo.sent_time );
   fc::raw::pack( enc, shared_secret );
   auto encrypt_key = enc.result();
   auto hash_encrypt_key = fc::sha256::hash( encrypt_key );
   pmo.checksum = hash_encrypt_key._hash[0];

   vector<char> plain_text = fc::raw::pack_to_vector( message );
   pmo.encrypted_message = fc::aes_encrypt( encrypt_key, plain_text );

   message_object obj;
   obj.to_memo_key   = pmo.to_memo_key;
   obj.from_memo_key = pmo.from_memo_key;
   obj.checksum = pmo.checksum;
   obj.sent_time = pmo.sent_time;
   obj.encrypted_message = pmo.encrypted_message;
   auto decrypted = try_decrypt_message(obj);

   op.data = fc::raw::pack_to_vector( pmo );

   signed_transaction tx;
   tx.operations.push_back( op );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}
message_body wallet_api::try_decrypt_message( const message_object& mo ) {
   message_body result;

   fc::sha512 shared_secret;

   auto it = my->_keys.find(mo.from_memo_key);
   if( it == my->_keys.end() )
   {
      it = my->_keys.find(mo.to_memo_key);
      if( it == my->_keys.end() )
      {
         wlog( "unable to find keys" );
         return result;
      }
      auto priv_key = wif_to_key( it->second );
      if( !priv_key ) return result;
      shared_secret = priv_key->get_shared_secret( mo.from_memo_key );
   } else {
      auto priv_key = wif_to_key( it->second );
      if( !priv_key ) return result;
      shared_secret = priv_key->get_shared_secret( mo.to_memo_key );
   }


   fc::sha512::encoder enc;
   fc::raw::pack( enc, mo.sent_time );
   fc::raw::pack( enc, shared_secret );
   auto encrypt_key = enc.result();

   uint32_t check = fc::sha256::hash( encrypt_key )._hash[0];

   if( mo.checksum != check )
      return result;

   auto decrypt_data = fc::aes_decrypt( encrypt_key, mo.encrypted_message );
   try {
      return fc::raw::unpack_from_vector<message_body>( decrypt_data );
   } catch ( ... ) {
      return result;
   }
}

vector<extended_message_object>   wallet_api::get_inbox( string account, fc::time_point newest, uint32_t limit ) {
   FC_ASSERT( !is_locked() );
   vector<extended_message_object> result;
   auto remote_result = (*my->_remote_message_api)->get_inbox( account, newest, limit );
   for( const auto& item : remote_result ) {
      result.emplace_back( item );
      result.back().message = try_decrypt_message( item );
   }
   return result;
}

vector<extended_message_object>   wallet_api::get_outbox( string account, fc::time_point newest, uint32_t limit ) {
   FC_ASSERT( !is_locked() );
   vector<extended_message_object> result;
   auto remote_result = (*my->_remote_message_api)->get_outbox( account, newest, limit );
   for( const auto& item : remote_result ) {
      result.emplace_back( item );
      result.back().message = try_decrypt_message( item );
   }
   return result;
}

vector <report_object> wallet_api::get_reports(string consumer)
{
   return my->_remote_db->get_reports_for_account(consumer); 
}

uint64_t wallet_api::get_account_scoring( string account )
{
   return my->_remote_db->get_account_scoring(account);
}
uint64_t wallet_api::get_content_scoring( string content )
{
   return my->_remote_db->get_content_scoring(content);
}

annotated_signed_transaction wallet_api::create_asset(string issuer, string asset_name, string description, uint64_t max_supply, bool broadcast)
{
   return my->create_asset(issuer, asset_name, description, max_supply, broadcast); 
}

annotated_signed_transaction wallet_api::issue_asset(string asset_name, string to_account, fc::real128 amount, bool broadcast)
{
   return my->issue_asset(asset_name, to_account, amount, broadcast);
}

annotated_signed_transaction wallet_api::reserve_asset(string asset_name, string from_account, fc::real128 amount, bool broadcast)
{
   return my->reserve_asset(asset_name, from_account, amount, broadcast);
}

annotated_signed_transaction wallet_api::update_asset(string asset_name, string description, uint64_t max_supply, string new_issuer, bool broadcast)
{
   return my->update_asset(asset_name, description, max_supply, new_issuer, broadcast);
}

optional<asset_object>  wallet_api::get_asset_details(string asset_name)
{
   return my->find_asset(asset_name);
}

vector<asset_object> wallet_api::list_assets( uint32_t start_id, uint32_t limit)
{
   return my->list_assets( start_id, limit );
}

vector <account_balance_object> wallet_api::get_assets (string account)
{
   return my->get_assets( account );
}


transaction_handle_type wallet_api::begin_builder_transaction()
{
   return my->begin_builder_transaction();
}

void wallet_api::add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op)
{
   my->add_operation_to_builder_transaction(transaction_handle, op);
}

void wallet_api::replace_operation_in_builder_transaction(transaction_handle_type handle, unsigned operation_index, const operation& new_op)
{
   my->replace_operation_in_builder_transaction(handle, operation_index, new_op);
}

transaction wallet_api::preview_builder_transaction(transaction_handle_type handle)
{
   return my->preview_builder_transaction(handle);
}

signed_transaction wallet_api::sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast)
{
   return my->sign_builder_transaction(transaction_handle, broadcast);
}

signed_transaction wallet_api::propose_builder_transaction(
      transaction_handle_type handle,
      time_point_sec expiration,
      uint32_t review_period_seconds,
      bool broadcast)
{
   return my->propose_builder_transaction(handle, expiration, review_period_seconds, broadcast);
}

signed_transaction wallet_api::propose_builder_transaction2(
      transaction_handle_type handle,
      string account_name_or_id,
      time_point_sec expiration,
      uint32_t review_period_seconds,
      bool broadcast)
{
   return my->propose_builder_transaction2(handle, account_name_or_id, expiration, review_period_seconds, broadcast);
}

void wallet_api::remove_builder_transaction(transaction_handle_type handle)
{
   return my->remove_builder_transaction(handle);
}

signed_transaction wallet_api::approve_proposal(
      const string& proposal_id,
      const approval_delta& delta,
      bool broadcast /* = false */
      )
{
   return my->approve_proposal( proposal_id, delta, broadcast );
}

} } // muse::wallet

