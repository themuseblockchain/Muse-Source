#include <muse/chain/protocol/asset.hpp>
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace muse { namespace chain {
      typedef boost::multiprecision::int128_t  int128_t;

      uint8_t asset::decimals()const {
         return MUSE_ASSET_PRECISION;
      }

      string asset::to_string()const {
         int64_t init_digits=amount.value/(precision());
         uint64_t fraction=amount.value%(precision());
         string output=std::to_string(init_digits);
         output=output+"."+std::to_string(precision() + fraction).erase(0,1);
         object_id_type id_t(asset_id);
         output = output +" "+ std::string(id_t);
         return output;
      }

      asset asset::from_string(const string& from) {
         int64_t amount;
         string s = fc::trim( from );
         auto dot_pos = s.find( "." );
         auto space_pos = s.find( " " );
         FC_ASSERT ( space_pos != std::string::npos );
         if(space_pos < dot_pos) { //no dot in the first part
            auto intpart = s.substr( 0, space_pos );
            amount = fc::to_int64(intpart)*static_precision();
         }else{

            auto intpart = s.substr( 0, dot_pos );
            amount = fc::to_int64(intpart)*static_precision();

            std::string fractpart = s.substr( dot_pos+1, std::min<size_t>(space_pos-dot_pos-1, MUSE_ASSET_PRECISION));
            while (fractpart.size() < MUSE_ASSET_PRECISION)
               fractpart+='0';

            uint64_t fract_amount = fc::to_int64(fractpart);

            amount = amount + fract_amount;
         }

         auto asset_id_s = s.substr( space_pos+1 );
         auto first_dot = asset_id_s.find('.');
         auto second_dot = asset_id_s.find('.',first_dot+1);
         FC_ASSERT( first_dot != second_dot );
         FC_ASSERT( first_dot != 0 && first_dot != std::string::npos );
         uint64_t number = fc::to_uint64(asset_id_s.substr( second_dot+1 ));

         FC_ASSERT( number <= GRAPHENE_DB_MAX_INSTANCE_ID );

         return asset(amount, asset_id_type(number));

      }

      share_type asset::scaled_precision( uint8_t decimals ){
         FC_ASSERT(decimals<19);
         share_type res=1;
         for (int i=0; i< decimals; i++)
            res*=10;
         return res;
      }

      int64_t asset::static_precision(){
         return 1000000;
      }

      int64_t asset::precision()const {
         return 1000000;
      }

      bool operator == ( const price& a, const price& b )
      {
         if( std::tie( a.base.asset_id, a.quote.asset_id ) != std::tie( b.base.asset_id, b.quote.asset_id ) )
             return false;

         const auto amult = uint128_t( b.quote.amount.value ) * a.base.amount.value;
         const auto bmult = uint128_t( a.quote.amount.value ) * b.base.amount.value;

         return amult == bmult;
      }

      bool operator < ( const price& a, const price& b )
      {
         if( a.base.asset_id < b.base.asset_id ) return true;
         if( a.base.asset_id > b.base.asset_id ) return false;
         if( a.quote.asset_id < b.quote.asset_id ) return true;
         if( a.quote.asset_id > b.quote.asset_id ) return false;

         const auto amult = uint128_t( b.quote.amount.value ) * a.base.amount.value;
         const auto bmult = uint128_t( a.quote.amount.value ) * b.base.amount.value;

         return amult < bmult;
      }

      bool operator <= ( const price& a, const price& b )
      {
         return (a == b) || (a < b);
      }

      bool operator != ( const price& a, const price& b )
      {
         return !(a == b);
      }

      bool operator > ( const price& a, const price& b )
      {
         return !(a <= b);
      }

      bool operator >= ( const price& a, const price& b )
      {
         return !(a < b);
      }

      asset operator * ( const asset& a, const price& b )
      {
         if( a.asset_id == b.base.asset_id )
         {
            FC_ASSERT( b.base.amount.value > 0 );
            uint128_t result = (uint128_t(a.amount.value) * b.quote.amount.value)/b.base.amount.value;
            FC_ASSERT( result.hi == 0 );
            return asset( result.to_uint64(), b.quote.asset_id );
         }
         else if( a.asset_id == b.quote.asset_id )
         {
            FC_ASSERT( b.quote.amount.value > 0 );
            uint128_t result = (uint128_t(a.amount.value) * b.base.amount.value)/b.quote.amount.value;
            FC_ASSERT( result.hi == 0 );
            return asset( result.to_uint64(), b.base.asset_id );
         }
         FC_THROW_EXCEPTION( fc::assert_exception, "invalid asset * price", ("asset",a)("price",b) );
      }

      price operator / ( const asset& base, const asset& quote )
      { try {
         FC_ASSERT( base.asset_id != quote.asset_id );
         return price{ base, quote };
      } FC_CAPTURE_AND_RETHROW( (base)(quote) ) }

      price price::max( asset_id_type base, asset_id_type quote ) { return asset( share_type(MUSE_MAX_SHARE_SUPPLY), base ) / asset( share_type(1), quote); }
      price price::min( asset_id_type base, asset_id_type quote ) { return asset( 1, base ) / asset( MUSE_MAX_SHARE_SUPPLY, quote); }

      bool price::is_null() const { return *this == price(); }

      void price::validate() const
      { try {
         FC_ASSERT( base.amount > share_type(0) );
         FC_ASSERT( quote.amount > share_type(0) );
         FC_ASSERT( base.asset_id != quote.asset_id );
      } FC_CAPTURE_AND_RETHROW( (base)(quote) ) }


} } // muse::chain
