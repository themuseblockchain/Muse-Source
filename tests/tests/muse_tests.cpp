/*
 * Copyright (c) 2017 Peertracks, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/test/unit_test.hpp>

#include <muse/app/database_api.hpp>

#include "../common/database_fixture.hpp"

using namespace muse::chain;
using namespace graphene::db;

BOOST_FIXTURE_TEST_SUITE( muse_tests, clean_database_fixture )

#define FAIL( msg, op ) \
   BOOST_TEST_MESSAGE( "--- Test failure " # msg ); \
   tx.operations.clear(); \
   tx.operations.push_back( op ); \
   MUSE_REQUIRE_THROW( db.push_transaction( tx, database::skip_transaction_signatures ), fc::assert_exception )

BOOST_AUTO_TEST_CASE( simple_test )
{
   try
   {
      generate_blocks( time_point_sec( MUSE_HARDFORK_0_1_TIME ) );
      BOOST_CHECK( db.has_hardfork( MUSE_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Testing: streaming platform contract" );

      muse::app::database_api dbapi(db);

      ACTORS( (suzy)(uhura)(paula)(penny)(martha)(muriel)(colette)(veronica)(vici) );

      generate_block();

      signed_transaction tx;
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );

      // --------- Create streaming platform ------------
      {
      streaming_platform_update_operation spuo;
      spuo.fee = asset( MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE, MUSE_SYMBOL );
      spuo.owner = "suzy";
      spuo.url = "http://www.google.de";
      tx.operations.push_back( spuo );

      FAIL( "when insufficient funds for fee", spuo );

      fund( "suzy", MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE );

      spuo.fee = asset( 10, MUSE_SYMBOL );
      FAIL( "when fee too low", spuo );

      spuo.fee = asset( MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE, MUSE_SYMBOL );
      spuo.owner = "x";
      FAIL( "with bad account", spuo );

      spuo.owner = "suzy";
      spuo.url = "";
      FAIL( "without url", spuo );

      spuo.url = "1234567890+++"; // MUSE_MAX_STREAMING_PLATFORM_URL_LENGTH
      for( int i = 0; i < MUSE_MAX_STREAMING_PLATFORM_URL_LENGTH / 10; i++ )
          spuo.url += "1234567890";
      FAIL( "with too long url", spuo );

      BOOST_TEST_MESSAGE( "--- Test success" );
      spuo.url = "http://www.google.de";
      tx.operations.clear();
      tx.operations.push_back( spuo );
      db.push_transaction( tx, database::skip_transaction_signatures  );
      }
      // --------- Look up streaming platforms ------------
      {
      set<string> sps = dbapi.lookup_streaming_platform_accounts("x", 5);
      BOOST_CHECK( sps.empty() );

      sps = dbapi.lookup_streaming_platform_accounts("", 5);
      BOOST_CHECK_EQUAL( 1, sps.size() );
      BOOST_CHECK( sps.find("suzy") != sps.end() );
      }
      const streaming_platform_object& suzys = db.get_streaming_platform( "suzy" );

      // --------- Create content ------------

      {
      content_operation cop;
      cop.uploader = "uhura";
      cop.url = "ipfs://abcdef1";
      cop.album_meta.album_title = "First test song";
      cop.track_meta.track_title = "First test song";
      cop.comp_meta.third_party_publishers = false;
      distribution dist;
      dist.payee = "paula";
      dist.bp = MUSE_100_PERCENT;
      cop.distributions.push_back( dist );
      management_vote mgmt;
      mgmt.voter = "martha";
      mgmt.percentage = 100;
      cop.management.push_back( mgmt );
      cop.management_threshold = 100;
      cop.playing_reward = 10;
      cop.publishers_share = 0;

      cop.uploader = "x";
      FAIL( "with bad account", cop );

      cop.uploader = "uhura";
      cop.url = "http://abcdef1";
      FAIL( "with bad url protocol", cop );
      cop.url = "";
      FAIL( "with empty url", cop );
      cop.url = "ipfs://1234567890";
      for( int i = 0; i < MUSE_MAX_URL_LENGTH / 10; i++ )
          cop.url += "1234567890";
      FAIL( "with too long url", cop );

      cop.url = "ipfs://abcdef1";
      cop.album_meta.album_title = "";
      FAIL( "with empty album title", cop );
      cop.album_meta.album_title = "Sixteen tons";
      for( int i = 0; i < 16; i++ )
          cop.album_meta.album_title += " are sixteen tons";
      FAIL( "with long album title", cop );

      cop.album_meta.album_title = "First test song";
      cop.track_meta.track_title = "";
      FAIL( "with empty track title", cop );
      cop.track_meta.track_title = "Sixteen tons";
      for( int i = 0; i < 16; i++ )
          cop.track_meta.track_title += " are sixteen tons";
      FAIL( "with long track title", cop );

      cop.track_meta.track_title = "First test song";
      cop.distributions.begin()->payee = "x";
      FAIL( "with invalid payee name", cop );
      cop.distributions.begin()->payee = "bob";
      FAIL( "with non-existing payee", cop );

      cop.distributions.begin()->payee = "paula";
      cop.distributions.begin()->bp = MUSE_100_PERCENT + 1;
      FAIL( "with invalid distribution", cop );

      cop.distributions.begin()->bp = MUSE_100_PERCENT;
      cop.management.begin()->voter = "x";
      FAIL( "with invalid voter name", cop );
      cop.management.begin()->voter = "bob";
      FAIL( "with non-existant voter", cop );

      cop.management.begin()->voter = "martha";
      cop.management.begin()->percentage = 101;
      FAIL( "with invalid voter percentage", cop );

      cop.management.begin()->percentage = 100;
      cop.playing_reward = MUSE_100_PERCENT + 1;
      FAIL( "with invalid playing reward", cop );

      cop.playing_reward = 10;
      cop.publishers_share = MUSE_100_PERCENT + 1;
      FAIL( "with invalid publisher's share", cop );

      cop.publishers_share = 0;
      BOOST_TEST_MESSAGE( "--- Test success" );
      tx.operations.clear();
      tx.operations.push_back( cop );
      db.push_transaction( tx, database::skip_transaction_signatures  );
      }

      // --------- Publish playtime ------------
      {
      streaming_platform_report_operation spro;
      spro.streaming_platform = "suzy";
      spro.consumer = "colette";
      spro.content = "ipfs://abcdef1";
      spro.play_time = 100;

      spro.streaming_platform = "x";
      FAIL( "with invalid platform name", spro );
      spro.streaming_platform = "bob";
      FAIL( "with non-existing platform", spro );

      spro.streaming_platform = "suzy";
      spro.consumer = "x";
      FAIL( "with invalid consumer name", spro );
      spro.consumer = "bob";
      FAIL( "with non-existing consumer", spro );

      spro.consumer = "colette";
      spro.content = "ipfs://no";
      FAIL( "with non-existing content", spro );

      spro.content = "ipfs://abcdef1";
      BOOST_TEST_MESSAGE( "--- Test success" );
      tx.operations.clear();
      tx.operations.push_back( spro );
      db.push_transaction( tx, database::skip_transaction_signatures  );
      }
      // --------- Verify playtime ------------
      {
      const content_object& song1 = db.get_content( "ipfs://abcdef1" );
      BOOST_CHECK_EQUAL( 100, colette_id(db).total_listening_time );
      BOOST_CHECK_EQUAL( 1, song1.times_played );
      BOOST_CHECK_EQUAL( 1, song1.times_played_24 );

      vector<report_object> reports = dbapi.get_reports_for_account( "colette" );
      BOOST_CHECK_EQUAL( 1, reports.size() );
      BOOST_CHECK_EQUAL( suzys.id, reports[0].streaming_platform );
      BOOST_CHECK_EQUAL( colette_id, reports[0].consumer );
      BOOST_CHECK_EQUAL( song1.id, reports[0].content );
      BOOST_CHECK_EQUAL( db.head_block_time().sec_since_epoch(), reports[0].created.sec_since_epoch() );
      BOOST_CHECK_EQUAL( 100, reports[0].play_time );
      }
      // --------- Content update ------------
      {
      content_update_operation cup;
      cup.side = content_update_operation::side_t::master;
      cup.url = "ipfs://abcdef1";

      cup.side = content_update_operation::side_t::publisher;
      FAIL( "of publisher update for single-sided content", cup );

      cup.side = content_update_operation::side_t::master;
      cup.url = "ipfs://no";
      FAIL( "of update for non-existant url", cup );

      cup.url = "ipfs://abcdef1";
      cup.new_playing_reward = MUSE_100_PERCENT + 1;
      FAIL( "of update with too high playing reward", cup );

      cup.new_playing_reward = 11;
      cup.new_publishers_share = MUSE_100_PERCENT + 1;
      FAIL( "of update with too high publishers share", cup );

      cup.new_publishers_share = 1;
      cup.album_meta = content_metadata_album_master();
      cup.album_meta->album_title = "";
      FAIL( "with empty album title", cup );
      cup.album_meta->album_title = "Sixteen tons";
      for( int i = 0; i < 16; i++ )
          cup.album_meta->album_title += " are sixteen tons";
      FAIL( "with long album title", cup );

      cup.album_meta->album_title = "Simple test album";
      cup.track_meta = content_metadata_track_master();
      cup.track_meta->track_title = "";
      FAIL( "with empty track title", cup );
      cup.track_meta->track_title = "Sixteen tons";
      for( int i = 0; i < 16; i++ )
          cup.track_meta->track_title += " are sixteen tons";
      FAIL( "with long track title", cup );

      cup.track_meta->track_title = "Simple test track";
      distribution dist;
      dist.payee = "penny";
      dist.bp = MUSE_100_PERCENT;
      cup.new_distributions.push_back( dist );
      cup.new_distributions[0].payee = "x";
      FAIL( "with invalid payee name", cup );
      cup.new_distributions[0].payee = "bob";
      FAIL( "with non-existing payee", cup );

      cup.new_distributions[0].payee = "penny";
      cup.new_distributions[0].bp = MUSE_100_PERCENT + 1;
      FAIL( "with invalid distribution", cup );

      cup.new_distributions[0].bp = MUSE_100_PERCENT;
      management_vote mgmt;
      mgmt.voter = "muriel";
      mgmt.percentage = 100;
      cup.new_management.push_back( mgmt );
      cup.new_management[0].voter = "x";
      FAIL( "with invalid voter name", cup );
      cup.new_management[0].voter = "bob";
      FAIL( "with non-existant voter", cup );

      cup.new_management[0].voter = "muriel";
      cup.new_management[0].percentage = 101;
      FAIL( "with invalid voter percentage", cup );

      cup.new_management[0].percentage = 100;
      BOOST_TEST_MESSAGE( "--- Test success" );
      tx.operations.clear();
      tx.operations.push_back( cup );
      db.push_transaction( tx, database::skip_transaction_signatures  );
      }
      // --------- Verify update ------------
      {
      const content_object& song1 = db.get_content( "ipfs://abcdef1" );
      BOOST_CHECK_EQUAL( "Simple test album", song1.album_meta.album_title );
      BOOST_CHECK_EQUAL( "Simple test track", song1.track_meta.track_title );
      BOOST_CHECK_EQUAL( "penny", song1.distributions_master[0].payee );
      BOOST_CHECK_EQUAL( 100, song1.manage_master.account_auths.at("muriel") );
      BOOST_CHECK_EQUAL( 11, song1.playing_reward );
      BOOST_CHECK_EQUAL( 1, song1.publishers_share );
      }
      // --------- Vote ------------
      {
         vote_operation vop;
         vop.voter = "veronica";
         vop.url = "ipfs://abcdef1";
         vop.weight = 1;

         vop.voter = "x";
         FAIL( "with bad account", vop );

         vop.voter = "veronica";
         vop.url = "http://abcdef1";
         FAIL( "with bad url protocol", vop );
         vop.url = "";
         FAIL( "with empty url", vop );
         vop.url = "ipfs://1234567890";
         for( int i = 0; i < MUSE_MAX_URL_LENGTH / 10; i++ )
            vop.url += "1234567890";
         FAIL( "with too long url", vop );

         vop.url = "ipfs://abcdef1";
         vop.weight = MUSE_100_PERCENT + 1;
         FAIL( "with bad weight", vop );

         vop.weight = 1;
         BOOST_TEST_MESSAGE( "--- Test success" );
         tx.operations.clear();
         tx.operations.push_back( vop );
         db.push_transaction( tx, database::skip_transaction_signatures  );

         vop.voter = "vici";
         tx.operations.clear();
         tx.operations.push_back( vop );
         db.push_transaction( tx, database::skip_transaction_signatures  );
      }

      BOOST_CHECK_EQUAL( 0, suzy_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).balance.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).mbd_balance.amount.value );

      BOOST_CHECK_EQUAL( 100000, suzy_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, uhura_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, paula_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, penny_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, martha_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, muriel_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, colette_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, veronica_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, vici_id(db).vesting_shares.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).curation_rewards.value );

      generate_blocks( db.head_block_time() + 86400 - MUSE_BLOCK_INTERVAL );

      asset daily_content_reward = asset( 1863530, MUSE_SYMBOL ); //db.get_content_reward();

      generate_block();

      {
      const auto& dgpo = db.get_dynamic_global_properties();
      asset curation_reserve = asset( daily_content_reward.amount.value / 10, MUSE_SYMBOL );
      daily_content_reward -= curation_reserve;
      asset platform_reward = asset( daily_content_reward.amount.value * 11 / MUSE_100_PERCENT, MUSE_SYMBOL ); // playing reward
      daily_content_reward -= platform_reward;
      asset comp_reward = asset( daily_content_reward.amount.value * 1 / MUSE_100_PERCENT, MUSE_SYMBOL ); // publishers_share
      asset master_reward = daily_content_reward - comp_reward;

      const content_object& song1 = db.get_content( "ipfs://abcdef1" );
      BOOST_CHECK_EQUAL( 0, song1.accumulated_balance_master.amount.value );
      BOOST_CHECK_EQUAL( comp_reward.amount.value, song1.accumulated_balance_comp.amount.value );
      BOOST_CHECK_EQUAL( comp_reward.asset_id, song1.accumulated_balance_comp.asset_id );
      BOOST_CHECK_EQUAL( master_reward.amount.value, penny_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 100000 + (platform_reward * dgpo.get_vesting_share_price()).amount.value, suzy_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( curation_reserve.amount.value / 10, veronica_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( ( curation_reserve.amount.value - curation_reserve.amount.value / 10 ) / 10, vici_id(db).balance.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).balance.amount.value );
      //BOOST_CHECK_EQUAL( 0, penny_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).balance.amount.value );
      //BOOST_CHECK_EQUAL( 0, veronica_id(db).balance.amount.value );
      //BOOST_CHECK_EQUAL( 0, vici_id(db).balance.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).mbd_balance.amount.value );

      //BOOST_CHECK_EQUAL( 100000, suzy_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, uhura_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, paula_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, penny_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, martha_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, muriel_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, colette_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, veronica_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, vici_id(db).vesting_shares.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).curation_rewards.value );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( multi_test )
{
   try
   {
      generate_blocks( time_point_sec( MUSE_HARDFORK_0_1_TIME ) );
      BOOST_CHECK( db.has_hardfork( MUSE_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Testing: streaming platform contract" );

      muse::app::database_api dbapi(db);

      ACTORS( (suzy)(uhura)(paula)(penny)(martha)(miranda)(muriel)(colette)(veronica)(vici) );

      generate_block();

      signed_transaction tx;
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );

      // --------- Create streaming platform ------------
      {
      fund( "suzy", MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE );

      streaming_platform_update_operation spuo;
      spuo.fee = asset( MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE, MUSE_SYMBOL );
      spuo.owner = "suzy";
      spuo.url = "http://www.google.de";
      tx.operations.clear();
      tx.operations.push_back( spuo );
      db.push_transaction( tx, database::skip_transaction_signatures  );
      }

      // --------- Create content ------------

      {
      content_operation cop;
      cop.uploader = "uhura";
      cop.url = "ipfs://abcdef9";
      cop.album_meta.album_title = "Multi test song";
      cop.track_meta.track_title = "Multi test song";
      cop.comp_meta.third_party_publishers = true;
      distribution dist;
      dist.payee = "paula";
      dist.bp = MUSE_100_PERCENT / 3;
      cop.distributions.push_back( dist );
      dist.payee = "penny";
      dist.bp = MUSE_100_PERCENT - dist.bp;
      cop.distributions.push_back( dist );
      management_vote mgmt;
      mgmt.voter = "martha";
      mgmt.percentage = 34;
      cop.management.push_back( mgmt );
      mgmt.voter = "miranda";
      mgmt.percentage = 33;
      cop.management.push_back( mgmt );
      mgmt.voter = "muriel";
      mgmt.percentage = 33;
      cop.management.push_back( mgmt );
      cop.management_threshold = 50;
      cop.playing_reward = 10;
      cop.publishers_share = 1000;
      cop.distributions_comp = vector<distribution>();
      dist.bp = MUSE_100_PERCENT;
      cop.distributions_comp->push_back(dist);
      cop.management_comp = vector<management_vote>();
      mgmt.percentage = 100;
      cop.management_comp->push_back(mgmt);
      cop.management_threshold_comp = 100;

      (*cop.distributions_comp)[0].payee = "x";
      FAIL( "with invalid payee name", cop );
      (*cop.distributions_comp)[0].payee = "bob";
      FAIL( "with non-existing payee", cop );

      (*cop.distributions_comp)[0].payee = "penny";
      (*cop.distributions_comp)[0].bp++;
      FAIL( "with invalid distribution", cop );

      (*cop.distributions_comp)[0].bp--;
      (*cop.management_comp)[0].voter = "x";
      FAIL( "with invalid voter name", cop );
      (*cop.management_comp)[0].voter = "bob";
      FAIL( "with non-existant voter", cop );

      (*cop.management_comp)[0].voter = "martha";
      (*cop.management_comp)[0].percentage++;
      FAIL( "with invalid voter percentage", cop );

      (*cop.management_comp)[0].percentage--;
      BOOST_TEST_MESSAGE( "--- Test success" );
      tx.operations.clear();
      tx.operations.push_back( cop );
      db.push_transaction( tx, database::skip_transaction_signatures  );
      }
      {
      const content_object& song1 = db.get_content( "ipfs://abcdef9" );
      BOOST_CHECK_EQUAL( 2, song1.distributions_master.size() );
      BOOST_CHECK_EQUAL( 1, song1.distributions_comp.size() );
      BOOST_CHECK_EQUAL( 3, song1.manage_master.num_auths() );
      BOOST_CHECK_EQUAL( 1, song1.manage_comp.num_auths() );
      }
      // --------- Publish playtime ------------
      {
      streaming_platform_report_operation spro;
      spro.streaming_platform = "suzy";
      spro.consumer = "colette";
      spro.content = "ipfs://abcdef9";
      spro.play_time = 100;

      BOOST_TEST_MESSAGE( "--- Test success" );
      tx.operations.clear();
      tx.operations.push_back( spro );
      db.push_transaction( tx, database::skip_transaction_signatures  );
      }

      // --------- Content update ------------
      {
      content_update_operation cup;
      cup.side = content_update_operation::side_t::publisher;
      cup.url = "ipfs://abcdef9";
      cup.new_playing_reward = 11;
      cup.new_publishers_share = 1;

      cup.album_meta = content_metadata_album_master();
      cup.album_meta->album_title = "Hello World";
      FAIL( "when publisher changes album metadata", cup );

      cup.album_meta.reset();
      cup.track_meta = content_metadata_track_master();
      cup.track_meta->track_title = "Hello World";
      FAIL( "when publisher changes track metadata", cup );

      cup.track_meta.reset();
      distribution dist;
      dist.payee = "penny";
      dist.bp = MUSE_100_PERCENT;
      cup.new_distributions.push_back( dist );
      management_vote mgmt;
      mgmt.voter = "muriel";
      mgmt.percentage = 100;
      cup.new_management.push_back( mgmt );

      cup.comp_meta = content_metadata_publisher();
      cup.comp_meta->third_party_publishers = false;
      BOOST_TEST_MESSAGE( "--- Test success" );
      tx.operations.clear();
      tx.operations.push_back( cup );
      db.push_transaction( tx, database::skip_transaction_signatures  );
      }
      // --------- Verify update ------------
      {
      const content_object& song1 = db.get_content( "ipfs://abcdef9" );
      BOOST_CHECK( song1.comp_meta.third_party_publishers );
      BOOST_CHECK_EQUAL( "penny", song1.distributions_comp[0].payee );
      BOOST_CHECK_EQUAL( 1, song1.distributions_comp.size() );
      BOOST_CHECK_EQUAL( 100, song1.manage_comp.account_auths.at("muriel") );
      BOOST_CHECK_EQUAL( 1, song1.manage_comp.num_auths() );
      BOOST_CHECK_EQUAL( 11, song1.playing_reward );
      BOOST_CHECK_EQUAL( 1, song1.publishers_share );
      }
      // --------- Vote ------------
      {
         vote_operation vop;
         vop.voter = "veronica";
         vop.url = "ipfs://abcdef9";
         vop.weight = 1;

         vop.voter = "x";
         FAIL( "with bad account", vop );

         vop.voter = "veronica";
         vop.url = "http://abcdef9";
         FAIL( "with bad url protocol", vop );
         vop.url = "";
         FAIL( "with empty url", vop );
         vop.url = "ipfs://1234567890";
         for( int i = 0; i < MUSE_MAX_URL_LENGTH / 10; i++ )
            vop.url += "1234567890";
         FAIL( "with too long url", vop );

         vop.url = "ipfs://abcdef9";
         vop.weight = MUSE_100_PERCENT + 1;
         FAIL( "with bad weight", vop );

         vop.weight = 1;
         BOOST_TEST_MESSAGE( "--- Test success" );
         tx.operations.clear();
         tx.operations.push_back( vop );
         db.push_transaction( tx, database::skip_transaction_signatures  );

         vop.voter = "vici";
         tx.operations.clear();
         tx.operations.push_back( vop );
         db.push_transaction( tx, database::skip_transaction_signatures  );
      }

      BOOST_CHECK_EQUAL( 0, suzy_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).balance.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).mbd_balance.amount.value );

      BOOST_CHECK_EQUAL( 100000, suzy_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, uhura_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, paula_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, penny_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, martha_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, muriel_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, colette_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, veronica_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, vici_id(db).vesting_shares.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).curation_rewards.value );

      generate_blocks( db.head_block_time() + 86400 - MUSE_BLOCK_INTERVAL );

      asset daily_content_reward = asset( 1863530, MUSE_SYMBOL ); //db.get_content_reward();

      generate_block();

      {
      const auto& dgpo = db.get_dynamic_global_properties();
      asset curation_reserve = asset( daily_content_reward.amount.value / 10, MUSE_SYMBOL );
      daily_content_reward -= curation_reserve;
      asset platform_reward = asset( daily_content_reward.amount.value * 11 / MUSE_100_PERCENT, MUSE_SYMBOL ); // playing reward
      daily_content_reward -= platform_reward;
      asset comp_reward = asset( daily_content_reward.amount.value * 1 / MUSE_100_PERCENT, MUSE_SYMBOL ); // publishers_share
      asset master_reward = daily_content_reward - comp_reward;

      const content_object& song1 = db.get_content( "ipfs://abcdef9" );
      BOOST_CHECK_EQUAL( 0, song1.accumulated_balance_master.amount.value );
      BOOST_CHECK_EQUAL( 0, song1.accumulated_balance_comp.amount.value );
      BOOST_CHECK_EQUAL( master_reward.amount.value * (MUSE_100_PERCENT/3) / MUSE_100_PERCENT, paula_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( comp_reward.amount.value + master_reward.amount.value * (MUSE_100_PERCENT - MUSE_100_PERCENT/3) / MUSE_100_PERCENT, penny_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 100000 + (platform_reward * dgpo.get_vesting_share_price()).amount.value, suzy_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( curation_reserve.amount.value / 10, veronica_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( ( curation_reserve.amount.value - curation_reserve.amount.value / 10 ) / 10, vici_id(db).balance.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).balance.amount.value );
      //BOOST_CHECK_EQUAL( 0, paula_id(db).balance.amount.value );
      //BOOST_CHECK_EQUAL( 0, penny_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).balance.amount.value );
      //BOOST_CHECK_EQUAL( 0, veronica_id(db).balance.amount.value );
      //BOOST_CHECK_EQUAL( 0, vici_id(db).balance.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).mbd_balance.amount.value );

      //BOOST_CHECK_EQUAL( 100000, suzy_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, uhura_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, paula_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, penny_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, martha_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, muriel_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, colette_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, veronica_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, vici_id(db).vesting_shares.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).curation_rewards.value );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( simple_authority_test )
{
   try
   {
      generate_blocks( time_point_sec( MUSE_HARDFORK_0_1_TIME ) );
      BOOST_CHECK( db.has_hardfork( MUSE_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Testing: streaming platform contract authority" );

      muse::app::database_api dbapi(db);

      ACTORS( (suzy)(uhura)(paula)(martha)(muriel)(colette) );

      generate_block();

      signed_transaction tx;
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );

      // --------- Create streaming platform ------------
      {
      fund( "suzy", MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE );
      streaming_platform_update_operation spuo;
      spuo.fee = asset( MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE, MUSE_SYMBOL );
      spuo.owner = "suzy";
      spuo.url = "http://www.google.de";
      tx.operations.push_back( spuo );
      tx.sign( uhura_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.signatures.clear();
      tx.sign( suzy_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );
      }

      // --------- Create content ------------
      {
      content_operation cop;
      cop.uploader = "uhura";
      cop.url = "ipfs://abcdef1";
      cop.album_meta.album_title = "First test song";
      cop.track_meta.track_title = "First test song";
      cop.comp_meta.third_party_publishers = false;
      distribution dist;
      dist.payee = "paula";
      dist.bp = MUSE_100_PERCENT;
      cop.distributions.push_back( dist );
      management_vote mgmt;
      mgmt.voter = "martha";
      mgmt.percentage = 100;
      cop.management.push_back( mgmt );
      cop.management_threshold = 100;
      cop.playing_reward = 10;
      cop.publishers_share = 0;
      tx.operations.clear();
      tx.operations.push_back( cop );
      tx.sign( suzy_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.signatures.clear();
      tx.sign( uhura_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );
      }

      // --------- Publish playtime ------------
      {
      streaming_platform_report_operation spro;
      spro.streaming_platform = "suzy";
      spro.consumer = "colette";
      spro.content = "ipfs://abcdef1";
      spro.play_time = 100;
      tx.operations.clear();
      tx.operations.push_back( spro );
      tx.sign( colette_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.signatures.clear();
      tx.sign( suzy_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );
      }

      // --------- Content update ------------
      {
      content_update_operation cup;
      cup.side = content_update_operation::side_t::master;
      cup.url = "ipfs://abcdef1";
      cup.new_playing_reward = 11;
      cup.new_publishers_share = 1;
      cup.album_meta = content_metadata_album_master();
      cup.album_meta->album_title = "Simple test album";
      cup.track_meta = content_metadata_track_master();
      cup.track_meta->track_title = "Simple test track";
      management_vote mgmt;
      mgmt.voter = "muriel";
      mgmt.percentage = 100;
      cup.new_management.push_back( mgmt );
      tx.operations.clear();
      tx.operations.push_back( cup );
      tx.sign( uhura_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.signatures.clear();
      tx.sign( muriel_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.signatures.clear();
      tx.sign( martha_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );
      }

      // --------- Content removal ------------
      {
      content_remove_operation cro;
      cro.url = "ipfs://abcdef1";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( cro );
      tx.sign( uhura_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.signatures.clear();
      tx.sign( martha_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.signatures.clear();
      tx.sign( muriel_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );
      }

      // --------- Wait for payout time and verify zero payout ------------

      generate_blocks( db.head_block_time() + 86400 - MUSE_BLOCK_INTERVAL );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).balance.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).mbd_balance.amount.value );

      BOOST_CHECK_EQUAL( 100000, suzy_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, uhura_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, paula_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, martha_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, muriel_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, colette_id(db).vesting_shares.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).curation_rewards.value );

      generate_block();

      BOOST_CHECK_EQUAL( 0, suzy_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).balance.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).mbd_balance.amount.value );

      BOOST_CHECK_EQUAL( 100000, suzy_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, uhura_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, paula_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, martha_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, muriel_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, colette_id(db).vesting_shares.amount.value );

      BOOST_CHECK_EQUAL( 0, suzy_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).curation_rewards.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).curation_rewards.value );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( multi_authority_test )
{
   try
   {
      generate_blocks( time_point_sec( MUSE_HARDFORK_0_1_TIME ) );
      BOOST_CHECK( db.has_hardfork( MUSE_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Testing: streaming platform contract authority" );

      muse::app::database_api dbapi(db);

      ACTORS( (suzy)(uhura)(paula)(martha)(miranda)(muriel)(colette) );

      generate_block();

      signed_transaction tx;
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );

      // --------- Create streaming platform ------------
      {
      fund( "suzy", MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE );
      streaming_platform_update_operation spuo;
      spuo.fee = asset( MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE, MUSE_SYMBOL );
      spuo.owner = "suzy";
      spuo.url = "http://www.google.de";
      tx.operations.push_back( spuo );
      tx.sign( uhura_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.signatures.clear();
      tx.sign( suzy_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );
      }

      // --------- Create content ------------
      {
      content_operation cop;
      cop.uploader = "uhura";
      cop.url = "ipfs://abcdef1";
      cop.album_meta.album_title = "First test song";
      cop.track_meta.track_title = "First test song";
      cop.comp_meta.third_party_publishers = true;
      distribution dist;
      dist.payee = "paula";
      dist.bp = MUSE_100_PERCENT;
      cop.distributions.push_back( dist );
      management_vote mgmt;
      mgmt.voter = "martha";
      mgmt.percentage = 34;
      cop.management.push_back( mgmt );
      mgmt.voter = "miranda";
      mgmt.percentage = 33;
      cop.management.push_back( mgmt );
      mgmt.voter = "muriel";
      mgmt.percentage = 33;
      cop.management.push_back( mgmt );
      cop.management_threshold = 50;
      cop.playing_reward = 10;
      cop.publishers_share = 0;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( cop );
      tx.sign( uhura_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );
      }

      // --------- Content update ------------
      {
      content_update_operation cup;
      cup.side = content_update_operation::side_t::master;
      cup.url = "ipfs://abcdef1";
      cup.album_meta = content_metadata_album_master();
      cup.album_meta->album_title = "Simple test album";
      cup.track_meta = content_metadata_track_master();
      cup.track_meta->track_title = "Simple test track";
      management_vote mgmt;
      mgmt.voter = "muriel";
      mgmt.percentage = 100;
      cup.new_management.push_back( mgmt );
      // FIXME
      mgmt.voter = "martha";
      cup.comp_meta = content_metadata_publisher();
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( cup );
      tx.sign( uhura_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.signatures.clear();
      tx.sign( muriel_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.sign( martha_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );
      }

      // --------- Another update ------------
      {
      content_update_operation cup;
      cup.side = content_update_operation::side_t::publisher;
      cup.url = "ipfs://abcdef1";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( cup );
      tx.sign( muriel_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.signatures.clear();
      tx.sign( martha_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );
      }

      // --------- Content removal ------------
      {
      content_remove_operation cro;
      cro.url = "ipfs://abcdef1";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( cro );
      tx.sign( uhura_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.signatures.clear();
      tx.sign( martha_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.signatures.clear();
      tx.sign( muriel_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_SUITE_END()
