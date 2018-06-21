#include <muse/chain/database.hpp>
#include <muse/chain/base_evaluator.hpp>
#include <muse/chain/base_objects.hpp>

#include <fc/uint128.hpp>
#include <fc/utf8.hpp>

#include <limits>

namespace muse { namespace chain {


void streaming_platform_update_evaluator::do_apply( const streaming_platform_update_operation& o )
{
   const auto& sp_account=db().get_account( o.owner ); // verify owner exists

   FC_ASSERT( o.url.size() <= MUSE_MAX_STREAMING_PLATFORM_URL_LENGTH );

   FC_ASSERT( sp_account.balance >= o.fee, "Insufficient balance to update streaming platform: have ${c}, need ${f}", ( "c", sp_account.balance )( "f", o.fee ) );

   const auto& by_streaming_platform_name_idx = db().get_index_type< streaming_platform_index >().indices().get< by_name >();
   auto wit_itr = by_streaming_platform_name_idx.find( o.owner );
   if( wit_itr != by_streaming_platform_name_idx.end() )
   {
      db().modify( *wit_itr, [&]( streaming_platform_object& w ) {
           w.url                = o.url;
      });
   }
   else
   {
      const witness_schedule_object& wso = db().get_witness_schedule_object();
      FC_ASSERT( o.fee >= wso.median_props.streaming_platform_update_fee, "Insufficient Fee: ${f} required, ${p} provided",
                 ("f", wso.median_props.streaming_platform_update_fee)
                       ("p", o.fee) );
      db().create< streaming_platform_object >( [&]( streaming_platform_object& w ) {
           w.owner              = o.owner;
           w.url                = o.url;
           w.created            = db().head_block_time();
      });
      db().pay_fee( sp_account, o.fee );

   }
}

void streaming_platform_report_evaluator::do_apply ( const streaming_platform_report_operation& o )
{
   const auto& consumer = db().get_account( o.consumer );
   FC_ASSERT( o.play_time + consumer.total_listening_time <= 86400, "User cannot cannot listen for more than 86400 seconds per day" );
   const auto& spidx = db().get_index_type<streaming_platform_index>().indices().get<by_name>();
   auto spitr = spidx.find(o.streaming_platform);
   FC_ASSERT(spitr != spidx.end());
   const auto& sp = * spitr;

   FC_ASSERT ( db().is_voted_streaming_platform( o.streaming_platform ));
   const auto& content = db().get_content( o.content );
   FC_ASSERT( !content.disabled );

   db().create< report_object>( [&](report_object& ro) {
        ro.consumer = consumer.id;
        ro.streaming_platform = sp.id;
        ro.created = db().head_block_time();
        ro.content = content.id;
        ro.play_time = o.play_time;
        if( o.playlist_creator.size() > 0 ){
           ro.playlist_creator = db().get_account(o.playlist_creator).id;
        }
   });

   db().modify< account_object >(consumer, [&]( account_object &a){
        a.total_listening_time += o.play_time;
   });

   db().modify< content_object >(content, [&] (content_object &c){
        ++c.times_played;
        ++c.times_played_24;
   });
}


void account_streaming_platform_vote_evaluator::do_apply( const account_streaming_platform_vote_operation& o )
{
   const auto& voter = db().get_account( o.account );

   const auto& streaming_platform = db().get_streaming_platform( o.streaming_platform );

   const auto& by_account_streaming_platform_idx = db().get_index_type< streaming_platform_vote_index >().indices().get< by_account_streaming_platform >();
   auto itr = by_account_streaming_platform_idx.find( boost::make_tuple( voter.get_id(), streaming_platform.get_id() ) );

   if( itr == by_account_streaming_platform_idx.end() ) {
      FC_ASSERT( o.approve, "vote doesn't exist, user must be indicate a desire to approve the streaming_platform" );
      FC_ASSERT( voter.streaming_platforms_voted_for < MUSE_MAX_ACCOUNT_WITNESS_VOTES, "account has voted for too many streaming_platforms");
      db().create<streaming_platform_vote_object> ( [&streaming_platform,&voter](streaming_platform_vote_object& v) {
           v.streaming_platform = streaming_platform.id;
           v.account = voter.id;
      });
      db().adjust_streaming_platform_vote( streaming_platform,  voter.witness_vote_weight());
      db().modify( voter, []( account_object& a ) {
           a.streaming_platforms_voted_for++;
      });

   } else {
      FC_ASSERT( !o.approve, "vote currently exists, user must indicate a desire to reject the streaming_platform" );

      db().adjust_streaming_platform_vote( streaming_platform,  -voter.witness_vote_weight());
      db().modify( voter, []( account_object& a ) {
           a.streaming_platforms_voted_for--;
      });
      db().remove( *itr );
   }
}




void friendship_evaluator::do_apply( const friendship_operation& o )
{
   const auto& a1 = db().get_account( o.who );
   const auto& a2 = db().get_account( o.whom );
   if( a1.friends.find( a2.id ) != a1.friends.end() ) //already friends
      return;
   if( a2.waiting.find( a1.id ) != a2.waiting.end() ) //repeated request
      return;
   if( a1.waiting.find( a2.id ) != a1.waiting.end() ) // approve friendship case
   {

      for( auto a3id : a2.friends ) {
         const auto &a3 = db().get<account_object>(a3id);
         db().modify<account_object>(a3, [&](account_object &a) {
              a.second_level.insert(a1.id);
         });
         db().recalculate_score(a3);
      }
      for( auto a3id : a1.friends ) {
         const auto &a3 = db().get<account_object>(a3id);
         db().modify<account_object>(a3, [&](account_object &a) {
              a.second_level.insert(a2.id);
         });
         db().recalculate_score(a3);
      }
      db().modify<account_object>( a1, [&]( account_object& a ){
           a.waiting.erase( a2.id );
           a.friends.insert( a2.id );
           a.second_level.insert( a2.friends.begin(), a2.friends.end() );
           a.second_level.erase( a.id );
      });
      db().recalculate_score(a1);
      db().modify<account_object>( a2, [&]( account_object& a ){
           a.friends.insert( a1.id );
           a.second_level.insert( a1.friends.begin(), a1.friends.end() ); //TODO_MUSE: potentially replace with set_union
           a.second_level.erase( a.id );
      });
      db().recalculate_score(a2);
      return;
   }

   db().modify<account_object>( a2, [&]( account_object& a ){
        a.waiting.insert( a1.id );
   });
}

void unfriend_evaluator::do_apply( const unfriend_operation& o )
{
   const auto& a1 = db().get_account( o.who );
   const auto& a2 = db().get_account( o.whom );
   if( a2.waiting.find( a1.id ) != a2.waiting.end() ) {
      db().modify<account_object>(a2, [&](account_object &a) {
           a.waiting.erase(a1.id);
      });
      return;
   }
   if( a1.waiting.find( a2.id ) != a1.waiting.end() ) {
      db().modify<account_object>(a1, [&](account_object &a) {
           a.waiting.erase(a2.id);
      });
      return;
   }
   /*for( auto aid : a1.friends ){

   }
   db().modify<account_object>( a1, [&]( account_object& a ){
        a.second_level.clear();
        for(auto aid : a.friends)
           a.second_level.insert( a2.friends.begin(), a2.friends.end() );
        a.second_level.erase( a.id );
   });*/
   if( a2.friends.find( a1.id ) != a2.friends.end() )
   {
      db().modify<account_object>( a2, [&]( account_object& a ) {
           a.friends.erase( a1.id );
           a.second_level.clear();
      });
      db().modify<account_object>( a1, [&]( account_object& a ) {
           a.friends.erase( a2.id );
           a.second_level.clear();
      });
      //rebuild second level lists
      set<account_id_type> new_sl_list;
      for( auto fid : a1.friends )
      {
         const auto& f = db().get<account_object>( fid );
         new_sl_list.insert( f.friends.begin(), f.friends.end() );
      }
      new_sl_list.erase( a1.id );
      db().modify<account_object>( a1, [&]( account_object& a ) {
           a.second_level = new_sl_list;
      });
      new_sl_list.clear();
      for( auto fid : a2.friends )
      {
         const auto& f = db().get<account_object>( fid );
         new_sl_list.insert( f.friends.begin(), f.friends.end() );
      }
      new_sl_list.erase( a2.id );
      db().modify<account_object>( a2, [&]( account_object& a ) {
           a.second_level = new_sl_list;
      });
      db().recalculate_score(a1);
      db().recalculate_score(a2);

      //rebuild second level lists of all friends. this is expensive
      for( auto aid : a1.friends )
      {
         const auto& f = db().get<account_object>( aid );
         set<account_id_type> new_sl_list;
         for( auto slid : f.friends )
         {
            const auto& sl = db().get<account_object>( slid );
            new_sl_list.insert( sl.friends.begin(), sl.friends.end() );
         }
         new_sl_list.erase( aid );
         db().modify<account_object>(f, [&](account_object& a) {
              a.second_level.clear();
              a.second_level = new_sl_list;
         });
         db().recalculate_score(f);
      }
      for( auto aid : a2.friends )
      {
         const auto& f = db().get<account_object>( aid );
         set<account_id_type> new_sl_list;
         for( auto slid : f.friends )
         {
            const auto& sl = db().get<account_object>( slid );
            new_sl_list.insert( sl.friends.begin(), sl.friends.end() );
         }
         new_sl_list.erase( aid );
         db().modify<account_object>(f, [&](account_object& a) {
              a.second_level.clear();
              a.second_level = new_sl_list;
         });
         db().recalculate_score(f);
      }
   }
}

void content_evaluator::do_apply( const content_operation& o )
{ try {

      const auto& by_url_idx = db().get_index_type< content_index >().indices().get< by_url >();
      auto itr = by_url_idx.find( o.url );

      FC_ASSERT( itr == by_url_idx.end(), "Content with given url already exists" );

      const auto& auth = db().get_account( o.uploader ); /// prove it exists

      FC_ASSERT( !(auth.owner_challenged || auth.active_challenged ) );

      for( const distribution& d : o.distributions )
         db().get_account( d.payee ); // ensure it exists

      if( o.distributions_comp )
         for( const distribution& d : *(o.distributions_comp) )
            db().get_account( d.payee ); // ensure it exists

      for( const management_vote& m : o.management )
         db().get_account(m.voter); // ensure it exists

      if( o.comp_meta.third_party_publishers  ){
         FC_ASSERT( o.management_comp && o.management_threshold_comp );
         for( const management_vote& m : *(o.management_comp) )
            db().get_account(m.voter); // ensure it exists
      }

      db().create< content_object >( [&o,this]( content_object& con ) {
           //validate_url
           con.uploader = o.uploader;
           con.url = o.url;

           con.album_meta = o.album_meta;
           con.track_meta = o.track_meta;
           con.comp_meta = o.comp_meta;
           con.track_title = o.track_meta.track_title;

           con.distributions_master = o.distributions;

           for( const management_vote& m : o.management )
           {
              con.manage_master.account_auths[m.voter] = m.percentage;
           }
           con.manage_master.weight_threshold = o.management_threshold;

           if(o.comp_meta.third_party_publishers)
           {
              for( const management_vote &m : *o.management_comp ) {
                 con.manage_comp.account_auths[m.voter] = m.percentage;
              }
              con.manage_comp.weight_threshold = *o.management_threshold_comp;

              if( o.distributions_comp )
                 con.distributions_comp = *(o.distributions_comp);

              if( db().has_hardfork( MUSE_HARDFORK_0_2 ) )
                 con.publishers_share = o.publishers_share;
           }
           else if( db().has_hardfork( MUSE_HARDFORK_0_2 ) )
               con.publishers_share = 0;
           con.accumulated_balance_master = asset(0);
           con.accumulated_balance_comp = asset(0);
           con.created = db().head_block_time();
           con.last_update = con.created;
           con.last_played = time_point_sec(0);
           con.times_played = 0;
           if( db().has_hardfork( MUSE_HARDFORK_0_2 ) )
              con.playing_reward = o.playing_reward;
      });
   } FC_CAPTURE_AND_RETHROW( (o) ) }

void content_update_evaluator::do_apply( const content_update_operation& o )
{ try {
      const auto& content = db().get_content( o.url );
      FC_ASSERT( !content.disabled );
      const content_object* itr = &content;

      bool two_sides = itr->comp_meta.third_party_publishers;
      if( db().has_hardfork(MUSE_HARDFORK_0_2) )
         FC_ASSERT( two_sides || o.side == o.master, "Cannot edit composition side data when only one side has been defined" );
      else
         FC_ASSERT( !two_sides || o.side == o.master, "Cannot edit composition side data when only one side has been defined" );

      for( const distribution& d : o.new_distributions )
         db().get_account( d.payee ); // just to ensure that d.payee account exists

      for( const management_vote& m : o.new_management )
         db().get_account(m.voter); // just to ensure that m.voter account exists

      asset accumulated_balances = (o.side==content_update_operation::side_t::master)?itr->accumulated_balance_master : itr->accumulated_balance_comp;
      db().modify< content_object >( *itr, [&o,this]( content_object& con ) {
           //the third_party_publishers flag cannot be changed. EVER.
           bool third_party_flag = con.comp_meta.third_party_publishers;
           if( o.side == o.master ) {
              if( o.album_meta )
                 con.album_meta = *o.album_meta;
              if( o.track_meta ) {
                 con.track_meta = *o.track_meta;
                 con.track_title = o.track_meta->track_title;
              }
              if( !third_party_flag && o.comp_meta )
                 con.comp_meta = *o.comp_meta;

              if( o.new_distributions.size() > 0 )
                 con.distributions_master = o.new_distributions;

              if( o.new_management.size() > 0 ) {
                 con.manage_master.account_auths.clear();
                 for( const management_vote &m : o.new_management ) {
                    con.manage_master.account_auths[m.voter] = m.percentage;
                 }
                 con.manage_master.weight_threshold = o.new_threshold;
              }
           }else{
              if( o.comp_meta ) {
                 con.comp_meta = *o.comp_meta;
              }
              if( o.new_distributions.size() > 0 )
                 con.distributions_comp = o.new_distributions;
              if( o.new_management.size() > 0 ) {
                 con.manage_comp.account_auths.clear();
                 for( const management_vote &m : o.new_management ) {
                    con.manage_comp.account_auths[m.voter] = m.percentage;
                 }
                 con.manage_comp.weight_threshold = o.new_threshold;
              }
           }
           con.comp_meta.third_party_publishers = third_party_flag;
           if( db().has_hardfork(MUSE_HARDFORK_0_2) ) {
              if( o.new_playing_reward > 0 )
                 con.playing_reward = o.new_playing_reward;
              if( o.new_publishers_share > 0 )
                 con.publishers_share = o.new_publishers_share;
           }else
           {
              con.playing_reward = o.new_playing_reward;
              con.publishers_share = o.new_publishers_share;
           }
           con.last_update = db().head_block_time();
      });
      if( o.new_distributions.size() > 0 && accumulated_balances.amount > 0 ) {
         if( o.side == o.master )
            db().pay_to_content_master( *itr, asset( 0, MUSE_SYMBOL ) );
         else
            db().pay_to_content_comp( *itr, asset( 0, MUSE_SYMBOL ) );
      }
   } FC_CAPTURE_AND_RETHROW( (o) ) }

void content_disable_evaluator::do_apply( const content_disable_operation& o )
{ try{
   const auto& content = db().get_content( o.url );

   FC_ASSERT( !content.disabled );

   db().modify( content, []( content_object& co ) {
       co.disabled = true;
   });

}  FC_CAPTURE_AND_RETHROW( (o) ) }

void content_approve_evaluator::do_apply( const content_approve_operation& o )
{try{
      const auto& content = db().get_content( o.url );
      FC_ASSERT( !content.disabled );

      const auto& appr = db().get_account( o.approver );

      db().create <content_approve_object> ( [&appr,&o](content_approve_object& con){
           con.approver=appr.id;
           con.content=o.url;
      });
   } FC_CAPTURE_AND_RETHROW( (o) ) }

void balance_claim_evaluator::do_apply( const balance_claim_operation& op )
{try{
      database& d = db();
      const auto& balance = d.get<balance_object>(op.balance_to_claim);

      const auto& gettie = d.get_account(op.deposit_to_account);

      FC_ASSERT(
            op.balance_owner_key == balance.owner ||
            pts_address(op.balance_owner_key, false, 56) == balance.owner ||
            pts_address(op.balance_owner_key, true, 56) == balance.owner ||
            pts_address(op.balance_owner_key, false, 0) == balance.owner ||
            pts_address(op.balance_owner_key, true, 0) == balance.owner ||
            address(op.balance_owner_key) == balance.owner,
            "Balance owner key was specified as '${op}' but balance's actual owner is '${bal}'",
            ("op", op.balance_owner_key)
            ("bal", balance.owner)
      );

      FC_ASSERT(op.total_claimed <= balance.balance);

      if( op.total_claimed < balance.balance )
         d.modify(balance, [&](balance_object& b) {
              b.balance -= op.total_claimed;
              b.last_claim_date = d.head_block_time();
         });
      else
         d.remove(balance);

      d.adjust_balance(gettie, op.total_claimed);
   }FC_CAPTURE_AND_RETHROW( (op) ) }

void vote_evaluator::do_apply( const vote_operation& o )
{ try {

      const auto& voter   = db().get_account( o.voter );
      FC_ASSERT( !(voter.owner_challenged || voter.active_challenged ) );

      auto elapsed_seconds   = (db().head_block_time() - voter.last_vote_time).to_seconds();
      FC_ASSERT( elapsed_seconds >= MUSE_MIN_VOTE_INTERVAL_SEC );

      const auto& now = db().head_block_time();
      db().modify( voter, [now]( account_object& a ){
           a.last_vote_time = std::move(now);
      });

      if( o.url.length() > 0 ) //vote for content
      {
         const auto&content = db().get_content( o.url );
         FC_ASSERT( !content.disabled );
         const auto weight = o.weight;
         if( weight > 0 ) FC_ASSERT( content.allow_votes );
         const auto& content_vote_idx = db().get_index_type< content_vote_index >().indices().get< by_content_voter >();
         auto itr = content_vote_idx.find( std::make_tuple( content.id, voter.id ) );

         if( itr!=content_vote_idx.end() ) //vote already exists...
         {
            FC_ASSERT( itr->num_changes < MUSE_MAX_VOTE_CHANGES, "Cannot change vote again" );

            FC_ASSERT( itr->weight != o.weight, "Changing your vote requires actually changing you vote." );

            db().modify( *itr, [weight,now]( content_vote_object& cv )
            {
                 cv.weight = weight;
                 cv.last_update = std::move(now);
                 cv.num_changes += 1;
            });
         }else{ //new vote...
            FC_ASSERT( weight != 0, "Weight cannot be 0");
            db().create<content_vote_object>( [&voter,&content,weight,now]( content_vote_object& cv ){
                 cv.voter=voter.id;
                 cv.content=content.id;
                 cv.weight = weight;
                 cv.last_update = std::move(now);
                 cv.num_changes = 0;
            });
         }
      }
   } FC_CAPTURE_AND_RETHROW( (o)) }


}}//namespace muse::chain
