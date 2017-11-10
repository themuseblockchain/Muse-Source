#pragma once
#include <muse/chain/protocol/types.hpp>
#include <muse/chain/config.hpp>

namespace muse { namespace chain {


   struct asset
   {
      string to_string()const;

      static asset from_string(const string& from);

      asset( share_type a = 0, asset_id_type id = MUSE_SYMBOL )
      :amount(a),asset_id(id){}

      share_type        amount;
      asset_id_type     asset_id;

      double to_real()const {
         return double(amount.value) / precision();
      }
      static share_type scaled_precision(uint8_t precision);

      uint8_t     decimals()const;
      int64_t     precision()const;
      static int64_t     static_precision();

      asset& operator += ( const asset& o )
      {
         FC_ASSERT( asset_id == o.asset_id );
         amount += o.amount;
         return *this;
      }

      asset& operator -= ( const asset& o )
      {
         FC_ASSERT( asset_id == o.asset_id );
         amount -= o.amount;
         return *this;
      }
      asset operator -()const { return asset( -amount, asset_id ); }

      friend bool operator == ( const asset& a, const asset& b )
      {
         return std::tie( a.asset_id, a.amount ) == std::tie( b.asset_id, b.amount );
      }
      friend bool operator < ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.asset_id == b.asset_id );
         return std::tie(a.amount,a.asset_id) < std::tie(b.amount,b.asset_id);
      }
      friend bool operator <= ( const asset& a, const asset& b )
      {
         return (a == b) || (a < b);
      }

      friend bool operator != ( const asset& a, const asset& b )
      {
         return !(a == b);
      }
      friend bool operator > ( const asset& a, const asset& b )
      {
         return !(a <= b);
      }
      friend bool operator >= ( const asset& a, const asset& b )
      {
         return !(a < b);
      }

      friend asset operator - ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.asset_id == b.asset_id );
         return asset( a.amount - b.amount, a.asset_id );
      }
      friend asset operator + ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.asset_id == b.asset_id );
         return asset( a.amount + b.amount, a.asset_id );
      }

      friend asset operator * ( const asset& a, uint32_t b ){
         return asset( a.amount*b, a.asset_id);
      }

      friend asset operator / ( const asset& a, uint32_t b ){
         return asset( a.amount / b, a.asset_id);
      }

 
   };

   struct price
   {
      price(const asset& base = asset(), const asset quote = asset())
         : base(base),quote(quote){}

      asset base;
      asset quote;

      static price max(asset_id_type base, asset_id_type quote );
      static price min(asset_id_type base, asset_id_type quote );


      price max()const { return price::max( base.asset_id, quote.asset_id ); }
      price min()const { return price::min( base.asset_id, quote.asset_id ); }

      static price unit_price(asset_id_type a = asset_id_type()) { return price(asset(1, a), asset(1, a)); }

      double to_real()const { return base.to_real() / quote.to_real(); }

      bool is_null()const;
      void validate()const;
   };

   price operator / ( const asset& base, const asset& quote );
   inline price operator~( const price& p ) { return price{p.quote,p.base}; }

   bool  operator <  ( const asset& a, const asset& b );
   bool  operator <= ( const asset& a, const asset& b );
   bool  operator <  ( const price& a, const price& b );
   bool  operator <= ( const price& a, const price& b );
   bool  operator >  ( const price& a, const price& b );
   bool  operator >= ( const price& a, const price& b );
   bool  operator == ( const price& a, const price& b );
   bool  operator != ( const price& a, const price& b );
   asset operator *  ( const asset& a, const price& b );


} }

namespace fc {
    inline void to_variant( const muse::chain::asset& var,  fc::variant& vo ) { 
       vo = var.to_string();
       //graphene::db::object_id_type object = var.asset_id;
       //vo += std::string(object);
    }
    inline void from_variant( const fc::variant& var,  muse::chain::asset& vo ) { vo = muse::chain::asset::from_string( var.as_string() ); }
}

FC_REFLECT( muse::chain::asset, (amount)(asset_id) )
FC_REFLECT( muse::chain::price, (base)(quote) )

