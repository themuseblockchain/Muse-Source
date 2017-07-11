#include <muse/chain/protocol/operations.hpp>

namespace fc {
   using namespace muse::chain;

   string name_from_type( string type_name ) {
      auto start = type_name.find_last_of( ':' )+1;
      auto end   = type_name.find_last_of( '_' );
      return type_name.substr( start, end-start );
   }
   struct from_operation
   {
      variant& var;
      from_operation( variant& dv ):var(dv){}

      typedef void result_type;
      template<typename T> void operator()( const T& v )const
      {
         auto name = name_from_type( fc::get_typename<T>::name() );
         var = variant( std::make_pair(name,v) );
      }
   };
   struct get_operation_name
   {
      string& name;
      get_operation_name( string& dv ):name(dv){}

      typedef void result_type;
      template<typename T> void operator()( const T& v )const
      {
         name = name_from_type( fc::get_typename<T>::name() );
      }
   };

    void to_variant( const muse::chain::operation& var,  fc::variant& vo )
    {
       var.visit( from_operation( vo ) );
    }
    void from_variant( const fc::variant& var,  muse::chain::operation& vo )
    {
      static std::map<string,uint32_t> to_tag = [](){
         std::map<string,uint32_t> name_map;
         for( int i = 0; i < operation::count(); ++i ) {
            operation tmp;
            tmp.set_which(i);
            string n;
            tmp.visit( get_operation_name(n) );
            name_map[n] = i;
         }
         return name_map;
      }();

      auto ar = var.get_array();
      if( ar.size() < 2 ) return;
      if( ar[0].is_uint64() )
         vo.set_which( ar[0].as_uint64() );
      else
      {
         auto itr = to_tag.find(ar[0].as_string());
         FC_ASSERT( itr != to_tag.end(), "Invalid operation name: ${n}", ("n", ar[0]) );
         vo.set_which( to_tag[ar[0].as_string()] );
      }
       vo.visit( fc::to_static_variant( ar[1] ) );
    }
}

namespace muse { namespace chain {


/**
 * @brief Used to validate operations in a polymorphic manner
 */
struct operation_validator
{
   typedef void result_type;
   template<typename T>
   void operator()( const T& v )const { v.validate(); }
};

struct operation_get_required_auth
{
   typedef void result_type;

   flat_set<string>&    active;
   flat_set<string>&    owner;
   flat_set<string>&    basic;
   flat_set<string>&    master_content;
   flat_set<string>&    comp_content;
   vector<authority>&   other;

   operation_get_required_auth( flat_set<string>& a,
     flat_set<string>& own,
     flat_set<string>& post,
     flat_set<string>& m_cont,
     flat_set<string>& c_cont,
     vector<authority>&  oth ):active(a),owner(own),basic(post),master_content(m_cont),comp_content(c_cont),other(oth){}

   template<typename T>
   void operator()( const T& v )const
   {
      wlog("getting authorities");
      v.get_required_active_authorities( active );
      v.get_required_owner_authorities( owner );
      v.get_required_basic_authorities( basic );
      v.get_required_master_content_authorities( master_content );
      v.get_required_comp_content_authorities( comp_content );
      v.get_required_authorities( other );
   }
};

void operation_validate( const operation& op )
{
   op.visit( operation_validator() );
}

void operation_get_required_authorities( const operation& op,
                                         flat_set<string>& active,
                                         flat_set<string>& owner,
                                         flat_set<string>& basic,
                                         flat_set<string>& master_content,
                                         flat_set<string>& comp_content,
                                         vector<authority>&  other )
{
   op.visit( operation_get_required_auth( active, owner, basic, master_content, comp_content, other ) );
}

struct is_market_op_visitor {
   typedef bool result_type;

   template<typename T>
   bool operator()( T&& v )const { return false; }
   bool operator()( const limit_order_create_operation& )const { return true; }
   bool operator()( const limit_order_cancel_operation& )const { return true; }
   bool operator()( const transfer_operation& )const { return true; }
   bool operator()( const transfer_to_vesting_operation& )const { return true; }
};

bool is_market_operation( const operation& op ) {
   return op.visit( is_market_op_visitor() );
}

struct is_proposal_op_visitor {
   typedef bool result_type;

   template<typename T>
   bool operator()( T&& v )const { return false; }
   bool operator()( const proposal_create_operation& )const { return true; }
};

bool is_proposal_operation( const operation& op ) {
   return op.visit( is_proposal_op_visitor() );
}

} } // namespace muse::chain
