#pragma once
#include <muse/chain/global_property_object.hpp>
#include <muse/chain/account_object.hpp>
#include <muse/chain/base_objects.hpp>

namespace muse { namespace app {
   using std::string;
   using std::vector;
   using namespace muse::chain;

   class extended_limit_order : public limit_order_object
   {
   public:
      extended_limit_order(){}
      explicit extended_limit_order( const limit_order_object& o ) : limit_order_object(o) {}

      double real_price  = 0;
      bool   rewarded    = false;
   };

   struct vote_state {
      string         voter;
      uint64_t       weight;
      int64_t        rshares;
      int16_t        percent;
      time_point_sec time;
   };

   struct account_vote {
      string         authorperm;
      uint64_t       weight;
      int64_t        rshares;
      int16_t        percent;
      time_point_sec time;
   };

   /**
    *  Convert's vesting shares
    */
   class extended_account : public account_object {
   public:
      extended_account(){}
      explicit extended_account( const account_object& a ) : account_object(a) {}

      asset                              muse_power; /// convert vesting_shares to vesting muse
      map<uint64_t,operation_object>     transfer_history; /// transfer to/from vesting
      map<uint64_t,operation_object>     market_history; /// limit order / cancel / fill
      map<uint64_t,operation_object>     vote_history;
      map<uint64_t,operation_object>     other_history;
      set<string>                        witness_votes;
      vector<proposal_object>            proposals;

      optional<map<uint32_t,extended_limit_order>> open_orders;
   };



   struct candle_stick {
      time_point_sec  open_time;
      uint32_t        period = 0;
      double          high = 0;
      double          low = 0;
      double          open = 0;
      double          close = 0;
      double          muse_volume = 0;
      double          dollar_volume = 0;
   };

   struct order_history_item {
      time_point_sec time;
      string         type; // buy or sell
      asset          mbd_quantity;
      asset          muse_quantity;
      double         real_price = 0;
   };

   struct market {
      vector<extended_limit_order> bids;
      vector<extended_limit_order> asks;
      vector<order_history_item>   history;
      vector<int>                  available_candlesticks;
      vector<int>                  available_zoom;
      int                          current_candlestick = 0;
      int                          current_zoom = 0;
      vector<candle_stick>         price_history;
   };

   /**
    *  This struct is designed
    */
   struct state {
        string                        current_route;

        dynamic_global_property_object props;

        /**
         *  map from account/slug to full nested discussion
         */
        map<string, extended_account> accounts;

        /**
         * The list of miners who are queued to produce work
         */
        vector<string>                pow_queue;
        map<string, witness_object>   witnesses;
        witness_schedule_object       witness_schedule;
        price                         feed_price;
        string                        error;
        optional<market>              market_data;
   };

} }

FC_REFLECT_DERIVED( muse::app::extended_account,
                   (muse::chain::account_object),
                   (muse_power)
                   (transfer_history)(market_history)(vote_history)(other_history)
                   (witness_votes)(open_orders)(proposals) )


FC_REFLECT( muse::app::vote_state, (voter)(weight)(rshares)(percent)(time) );
FC_REFLECT( muse::app::account_vote, (authorperm)(weight)(rshares)(percent)(time) );

FC_REFLECT( muse::app::state, (current_route)(props)(accounts)(pow_queue)(witnesses)(witness_schedule)(feed_price)(error)(market_data) )

FC_REFLECT_DERIVED( muse::app::extended_limit_order, (muse::chain::limit_order_object), (real_price)(rewarded) )
FC_REFLECT( muse::app::order_history_item, (time)(type)(mbd_quantity)(muse_quantity)(real_price) );
FC_REFLECT( muse::app::market, (bids)(asks)(history)(price_history)(available_candlesticks)(available_zoom)(current_candlestick)(current_zoom) )
FC_REFLECT( muse::app::candle_stick, (open_time)(period)(high)(low)(open)(close)(muse_volume)(dollar_volume) );
