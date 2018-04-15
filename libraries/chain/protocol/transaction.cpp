
#include <muse/chain/protocol/transaction.hpp>

#include <muse/chain/exceptions.hpp>

#include <fc/io/raw.hpp>
#include <fc/bitutil.hpp>
#include <fc/smart_ref_impl.hpp>

#include <algorithm>
#include <boost/container/detail/flat_tree.hpp>

namespace muse { namespace chain {

using namespace graphene::db;

digest_type signed_transaction::merkle_digest()const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, *this );
   return enc.result();
}

digest_type transaction::digest()const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, *this );
   return enc.result();
}

digest_type transaction::sig_digest( const chain_id_type& chain_id )const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, chain_id );
   fc::raw::pack( enc, *this );
   return enc.result();
}

void transaction::validate() const
{
   FC_ASSERT( operations.size() > 0, "A transaction must have at least one operation", ("trx",*this) );
   for( const auto& op : operations )
      operation_validate(op);
}

muse::chain::transaction_id_type muse::chain::transaction::id() const
{
   auto h = digest();
   transaction_id_type result;
   memcpy(result._hash, h._hash, std::min(sizeof(result), sizeof(h)));
   return result;
}

const signature_type& muse::chain::signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id)
{
   digest_type h = sig_digest( chain_id );
   signatures.push_back(key.sign_compact(h));
   return signatures.back();
}

signature_type muse::chain::signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id)const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, chain_id );
   fc::raw::pack( enc, *this );
   return key.sign_compact(enc.result());
}

void transaction::set_expiration( fc::time_point_sec expiration_time )
{
    expiration = expiration_time;
}

void transaction::set_reference_block( const block_id_type& reference_block )
{
   ref_block_num = fc::endian_reverse_u32(reference_block._hash[0]);
   ref_block_prefix = reference_block._hash[1];
}

void transaction::get_required_authorities( flat_set<string>& active,
                                            flat_set<string>& owner,
                                            flat_set<string>& basic,
                                            flat_set<string>& master_content,
                                            flat_set<string>& comp_content,
                                            vector<authority>& other )const
{
   for( const auto& op : operations )
      operation_get_required_authorities( op, active, owner, basic, master_content, comp_content, other );
}

void verify_authority_v1( const vector<operation>& ops, const flat_set<public_key_type>& sigs,
                       const authority_getter& get_active,
                       const authority_getter& get_owner,
                       const authority_getter& get_basic,
                       const authority_getter& get_master_content,
                       const authority_getter& get_comp_content,
                       uint32_t max_recursion_depth,
                       const flat_set<string>& active_aprovals,
                       const flat_set<string>& owner_approvals,
                       const flat_set<string>& basic_approvals
                       )
{ try {
   flat_set<string> required_active;
   flat_set<string> required_owner;
   flat_set<string> required_basic;
   flat_set<string> required_master_content;
   flat_set<string> required_comp_content;
   vector<authority> other;

   for( const auto& op : ops )
   {
      operation_get_required_authorities( op, required_active, required_owner, required_basic, required_master_content, required_comp_content, other );
   }
   /**
    *  Transactions with operations required basic authority cannot be combined
    *  with transactions requiring active or owner authority. This is for ease of
    *  implementation. Future versions of authority verification may be able to
    *  check for the merged authority of active and basic.
    */
   if( required_basic.size() ) {
      FC_ASSERT( required_active.size() == 0 );
      FC_ASSERT( required_owner.size() == 0 );
      FC_ASSERT( other.size() == 0 );

      sign_state s(sigs,get_basic);
      s.max_recursion = max_recursion_depth;
      for( auto& id : basic_approvals )
         s.approved_by.insert( id );

      for( auto id : required_basic )
      {
         MUSE_ASSERT( s.check_authority(id) ||
                          s.check_authority(get_active(id)) ||
                          s.check_authority(get_owner(id)),
                          tx_missing_basic_auth, "Missing Posting Authority ${id}",
                          ("id",id)
                          ("basic",*get_basic(id))
                          ("active",*get_active(id))
                          ("owner",*get_owner(id)) );
      }
      MUSE_ASSERT(
         !s.remove_unused_signatures(),
         tx_irrelevant_sig,
         "Unnecessary signature(s) detected"
         );
      return;
   }

   sign_state s(sigs,get_active);
   s.max_recursion = max_recursion_depth;
   for( auto& id : active_aprovals )
      s.approved_by.insert( id );
   for( auto& id : owner_approvals )
      s.approved_by.insert( id );

   for( const auto& auth : other )
   {
      MUSE_ASSERT( s.check_authority(&auth), tx_missing_other_auth, "Missing Authority", ("auth",auth)("sigs",sigs) );
   }

   // fetch all of the top level authorities
   for( auto id : required_active )
   {
      MUSE_ASSERT( s.check_authority(id) ||
                       s.check_authority(get_owner(id)),
                                tx_missing_active_auth, "Missing Active Authority ${id}", ("id",id)("auth",*get_active(id))("owner",*get_owner(id)) );
   }

   for( auto con : required_master_content )
   {
      MUSE_ASSERT( s.check_authority(get_master_content(con)),
                       tx_missing_active_auth, "Missing Content Authority for content ${con}", ("con", con)("auth",*get_master_content(con)));
   }

   for( auto con : required_comp_content )
   {
      MUSE_ASSERT( s.check_authority(get_comp_content(con)),
                       tx_missing_active_auth, "Missing Content Authority for content ${con}", ("con", con)("auth",*get_comp_content(con)));
   }

   for( auto id : required_owner )
   {
      MUSE_ASSERT( owner_approvals.find(id) != owner_approvals.end() ||
                       s.check_authority(get_owner(id)),
                       tx_missing_owner_auth, "Missing Owner Authority ${id}", ("id",id)("auth",*get_owner(id)) );
   }
} FC_CAPTURE_AND_RETHROW( (ops)(sigs) ) }

void verify_authority_v2( const vector<operation>& ops, const flat_set<public_key_type>& sigs,
                       const authority_getter& get_active,
                       const authority_getter& get_owner,
                       const authority_getter& get_basic,
                       const authority_getter& get_master_content,
                       const authority_getter& get_comp_content,
                       bool allow_extra_sigs,
                       uint32_t max_recursion_depth,
                       const flat_set<string>& active_aprovals,
                       const flat_set<string>& owner_approvals,
                       const flat_set<string>& basic_approvals
                       )
{ try {
   flat_set<string> required_active;
   flat_set<string> required_owner;
   flat_set<string> required_basic;
   flat_set<string> required_master_content;
   flat_set<string> required_comp_content;
   vector<authority> other;

   for( const auto& op : ops )
      operation_get_required_authorities( op, required_active, required_owner, required_basic, required_master_content, required_comp_content, other );

   // Active or owner authorities also cover basic authority
   for( const string& a : required_active )
      required_basic.erase( a );
   for( const string& o : required_owner )
      required_basic.erase( o );

   FC_ASSERT( required_basic.size() == 0
              || required_active.size() == 0
                 && required_owner.size() == 0
                 && required_master_content.size() == 0
                 && required_comp_content.size() == 0
                 && other.size() == 0,
              "Cannot combine operations with basic approval and others!" );

   sign_state s(sigs,get_active);
   s.max_recursion = max_recursion_depth;
   for( auto& id : basic_approvals )
      s.approved_by.insert( id );
   for( auto& id : active_aprovals )
      s.approved_by.insert( id );
   for( auto& id : owner_approvals )
      s.approved_by.insert( id );

   for( const auto& auth : other )
   {
      MUSE_ASSERT( s.check_authority(&auth), tx_missing_other_auth, "Missing Authority", ("auth",auth)("sigs",sigs) );
   }

   // fetch all of the top level authorities
   for( auto id : required_basic )
   {
      MUSE_ASSERT( s.check_authority(get_basic(id)) ||
                       s.check_authority(get_active(id)) ||
                       s.check_authority(get_owner(id)),
                                tx_missing_basic_auth, "Missing Active Authority ${id}", ("id",id)("auth",*get_active(id))("owner",*get_owner(id)) );
   }

   for( auto id : required_active )
   {
      MUSE_ASSERT( s.check_authority(id) ||
                       s.check_authority(get_owner(id)),
                                tx_missing_active_auth, "Missing Active Authority ${id}", ("id",id)("auth",*get_active(id))("owner",*get_owner(id)) );
   }

   for( auto con : required_master_content )
   {
      MUSE_ASSERT( s.check_authority(get_master_content(con)),
                       tx_missing_active_auth, "Missing Content Authority for content ${con}", ("con", con)("auth",*get_master_content(con)));
   }

   for( auto con : required_comp_content )
   {
      MUSE_ASSERT( s.check_authority(get_comp_content(con)),
                       tx_missing_active_auth, "Missing Content Authority for content ${con}", ("con", con)("auth",*get_comp_content(con)));
   }

   for( auto id : required_owner )
   {
      MUSE_ASSERT( owner_approvals.find(id) != owner_approvals.end() ||
                       s.check_authority(get_owner(id)),
                       tx_missing_owner_auth, "Missing Owner Authority ${id}", ("id",id)("auth",*get_owner(id)) );
   }

   if( !allow_extra_sigs )
      MUSE_ASSERT(
         !s.remove_unused_signatures(),
         tx_irrelevant_sig,
         "Unnecessary signature(s) detected"
         );
} FC_CAPTURE_AND_RETHROW( (ops)(sigs) ) }

flat_set<public_key_type> signed_transaction::get_signature_keys( const chain_id_type& chain_id )const
{ try {
   auto d = sig_digest( chain_id );
   flat_set<public_key_type> result;
   for( const auto&  sig : signatures )
   {
      MUSE_ASSERT(
         result.insert( fc::ecc::public_key(sig,d) ).second,
         tx_duplicate_sig,
         "Duplicate Signature detected" );
   }
   return result;
} FC_CAPTURE_AND_RETHROW() }



set<public_key_type> signed_transaction::get_required_signatures(
   const chain_id_type& chain_id,
   const flat_set<public_key_type>& available_keys,
   const authority_getter& get_active,
   const authority_getter& get_owner,
   const authority_getter& get_basic,
   const authority_getter& get_master_content,
   const authority_getter& get_comp_content,
   uint32_t max_recursion_depth )const
{
   flat_set<string> required_active;
   flat_set<string> required_owner;
   flat_set<string> required_basic;
   flat_set<string> required_master_content;
   flat_set<string> required_comp_content;

   vector<authority> other;
   get_required_authorities( required_active, required_owner, required_basic, required_master_content, required_comp_content, other );

   /** basic authority cannot be mixed with active authority in same transaction */
   if( required_basic.size() ) {
      sign_state s(get_signature_keys( chain_id ),get_basic,available_keys);
      s.max_recursion = max_recursion_depth;

      FC_ASSERT( !required_owner.size() );
      FC_ASSERT( !required_active.size() );
      for( auto& basic : required_basic )
         s.check_authority( basic  );

      s.remove_unused_signatures();

      set<public_key_type> result;

      for( auto& provided_sig : s.provided_signatures )
         if( available_keys.find( provided_sig.first ) != available_keys.end() )
            result.insert( provided_sig.first );

      return result;
   }


   sign_state s(get_signature_keys( chain_id ),get_active,available_keys);
   s.max_recursion = max_recursion_depth;

   for( const auto& auth : other )
      s.check_authority(&auth);
   for( auto& owner : required_owner )
      s.check_authority( get_owner( owner ) );
   for( auto& active : required_active )
      s.check_authority( active  );

   s.remove_unused_signatures();

   set<public_key_type> result;

   for( auto& provided_sig : s.provided_signatures )
      if( available_keys.find( provided_sig.first ) != available_keys.end() )
         result.insert( provided_sig.first );

   return result;
}

set<public_key_type> signed_transaction::minimize_required_signatures(
   const chain_id_type& chain_id,
   const flat_set<public_key_type>& available_keys,
   const authority_getter& get_active,
   const authority_getter& get_owner,
   const authority_getter& get_basic,
   const authority_getter& get_master_content,
   const authority_getter& get_comp_content,
   const uint32_t version,
   uint32_t max_recursion
   ) const
{
   set< public_key_type > s = get_required_signatures( chain_id, available_keys, get_active, get_owner, get_basic, get_master_content, get_comp_content, max_recursion );
   flat_set< public_key_type > result( s.begin(), s.end() );

   for( const public_key_type& k : s )
   {
      result.erase( k );
      try
      {
         switch( version )
         {
             case 1:
                verify_authority_v1( operations, result, get_active, get_owner, get_basic, get_master_content, get_comp_content, max_recursion );
                break;
             case 2:
                verify_authority_v2( operations, result, get_active, get_owner, get_basic, get_master_content, get_comp_content, false, max_recursion );
                break;
             default:
                FC_THROW_EXCEPTION( fc::invalid_arg_exception, "Unsupported version ${v}", ("v",version) );
         }
         continue;  // element stays erased if verify_authority is ok
      }
      catch( const tx_missing_owner_auth& e ) {}
      catch( const tx_missing_active_auth& e ) {}
      catch( const tx_missing_basic_auth& e ) {}
      catch( const tx_missing_other_auth& e ) {}
      result.insert( k );
   }
   return set<public_key_type>( result.begin(), result.end() );
}

void signed_transaction::verify_authority(
   const chain_id_type& chain_id,
   const authority_getter& get_active,
   const authority_getter& get_owner,
   const authority_getter& get_basic,
   const authority_getter& get_master_content,
   const authority_getter& get_comp_content,
   uint32_t version,
   uint32_t max_recursion)const
{ try {
   switch( version ) {
      case 1:
         muse::chain::verify_authority_v1( operations, get_signature_keys( chain_id ),
                                           get_active, get_owner, get_basic,
                                           get_master_content, get_comp_content,
                                           max_recursion );
         break;
      case 2:
         muse::chain::verify_authority_v2( operations, get_signature_keys( chain_id ),
                                           get_active, get_owner, get_basic,
                                           get_master_content, get_comp_content,
                                           false, max_recursion );
         break;
      default:
         FC_THROW_EXCEPTION( fc::invalid_arg_exception, "Unsupported version ${v}", ("v",version) );
   }
} FC_CAPTURE_AND_RETHROW( (*this) ) }

} } // muse::chain
