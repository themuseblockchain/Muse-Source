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

#include <graphene/utilities/tempdir.hpp>

#include "../common/database_fixture.hpp"

using namespace muse::chain;
using namespace graphene::db;

BOOST_FIXTURE_TEST_SUITE( muse_tests, clean_database_fixture )

#define FAIL( msg, op ) FAIL_WITH( msg, op, fc::assert_exception )

#define FAIL_WITH( msg, op, ex ) \
   BOOST_TEST_MESSAGE( "--- Test failure " # msg ); \
   tx.operations.clear(); \
   tx.operations.push_back( op ); \
   MUSE_REQUIRE_THROW( db.push_transaction( tx, database::skip_transaction_signatures ), ex )

BOOST_AUTO_TEST_CASE( streaming_platform_test )
{
   try
   {
      muse::app::database_api dbapi(db);

      generate_blocks( time_point_sec( MUSE_HARDFORK_0_1_TIME ) );
      BOOST_CHECK( db.has_hardfork( MUSE_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Testing: streaming platform contract" );

      ACTORS( (suzy)(victoria) );

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

      fund( "suzy", 2 * MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE );

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
      const streaming_platform_object& suzys = db.get_streaming_platform( "suzy" );
      BOOST_CHECK_EQUAL( "suzy", suzys.owner );
      BOOST_CHECK_EQUAL( db.head_block_time().sec_since_epoch(), suzys.created.sec_since_epoch() );
      BOOST_CHECK_EQUAL( "http://www.google.de", suzys.url );
      }

      const auto creation_time = db.head_block_time();

      generate_block();

      const streaming_platform_object& suzys = db.get_streaming_platform( "suzy" );
      BOOST_CHECK_EQUAL( "suzy", suzys.owner );
      BOOST_CHECK_EQUAL( creation_time.sec_since_epoch(), suzys.created.sec_since_epoch() );
      BOOST_CHECK_EQUAL( "http://www.google.de", suzys.url );

      // --------- Update streaming platform ------------
      {
      streaming_platform_update_operation spuo;
      spuo.fee = asset( MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE, MUSE_SYMBOL );
      spuo.owner = "suzy";
      spuo.url = "http://www.peertracks.com";
      tx.operations.clear();
      tx.operations.push_back( spuo );
      db.push_transaction( tx, database::skip_transaction_signatures  );
      }

      BOOST_CHECK_EQUAL( "suzy", suzys.owner );
      BOOST_CHECK_EQUAL( creation_time.sec_since_epoch(), suzys.created.sec_since_epoch() );
      BOOST_CHECK_EQUAL( "http://www.peertracks.com", suzys.url );

      // --------- Vote for streaming platform ------------
      {
         const account_object& vici = db.get_account( "victoria" );
         BOOST_CHECK_EQUAL( 0, vici.streaming_platforms_voted_for );
         BOOST_CHECK_EQUAL( 0, suzys.votes.value );

         account_streaming_platform_vote_operation aspvo;
         aspvo.account = "victoria";
         aspvo.streaming_platform = "suzy";
         aspvo.approve = true;

         aspvo.account = "x";
         FAIL( "with bad voting account", aspvo );

         aspvo.account = "victoria";
         aspvo.streaming_platform = "x";
         FAIL( "with bad streaming platform", aspvo );

         aspvo.streaming_platform = "suzy";
         aspvo.approve = false;
         FAIL( "with missing approval", aspvo );

         aspvo.approve = true;
         tx.operations.clear();
         tx.operations.push_back( aspvo );
         db.push_transaction( tx, database::skip_transaction_signatures );

         const auto& by_account_streaming_platform_idx = db.get_index_type< streaming_platform_vote_index >().indices().get< by_account_streaming_platform >();
         auto itr = by_account_streaming_platform_idx.find( boost::make_tuple( victoria_id, suzys.get_id() ) );

         BOOST_CHECK( itr != by_account_streaming_platform_idx.end() );
         BOOST_CHECK_EQUAL( victoria_id, itr->account );
         BOOST_CHECK_EQUAL( suzys.id, itr->streaming_platform );
         BOOST_CHECK_EQUAL( 1, vici.streaming_platforms_voted_for );
         BOOST_CHECK_EQUAL( vici.vesting_shares.amount.value, suzys.votes.value );

         tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION - 1 );
         FAIL( "with missing disapproval", aspvo );

         aspvo.approve = false;
         tx.operations.clear();
         tx.operations.push_back( aspvo );
         db.push_transaction( tx, database::skip_transaction_signatures  );

         BOOST_CHECK_EQUAL( 0, vici.streaming_platforms_voted_for );
         BOOST_CHECK_EQUAL( 0, suzys.votes.value );
      }

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( simple_test )
{
   try
   {
      generate_blocks( time_point_sec( MUSE_HARDFORK_0_2_TIME ) );
      BOOST_CHECK( db.has_hardfork( MUSE_HARDFORK_0_2 ) );

      BOOST_TEST_MESSAGE( "Testing: streaming platform contract" );

      muse::app::database_api dbapi(db);

      ACTORS( (alice)(suzy)(uhura)(paula)(penny)(martha)(muriel)(colette)(veronica)(vici) );

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
      cop.track_meta.json_metadata = "";
      FAIL( "with empty json metadata", cop );
      cop.track_meta.json_metadata = "{123: 123}";
      FAIL_WITH( "with invalid json metadata", cop, fc::parse_error_exception );
      cop.track_meta.json_metadata = "{\"id\": \"\200\"}";
      FAIL( "with non-utf8 json metadata", cop );
      cop.track_meta.json_metadata.reset();

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
      db.push_transaction( tx, database::skip_transaction_signatures );
      }

      // --------- Approve content ------------
      {
          content_approve_operation cao;
          cao.approver = "alice";
          cao.url = "ipfs://abcdef1";

          cao.approver = "x";
          FAIL( "with bad account", cao );

          cao.approver = "alice";
          cao.url = "http://abcdef1";
          FAIL( "with bad url protocol", cao );
          cao.url = "";
          FAIL( "with empty url", cao );
          cao.url = "ipfs://1234567890";
          for( int i = 0; i < MUSE_MAX_URL_LENGTH / 10; i++ )
              cao.url += "1234567890";
          FAIL( "with too long url", cao );

          cao.url = "ipfs://abcdef1";
          BOOST_TEST_MESSAGE( "--- Test success" );
          tx.operations.clear();
          tx.operations.push_back( cao );
          db.push_transaction( tx, database::skip_transaction_signatures );

          BOOST_TEST_MESSAGE( "--- Test failure with double approval" );
          tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION - 1 );
          tx.sign( alice_private_key, db.get_chain_id() );
          MUSE_REQUIRE_THROW( db.push_transaction( tx ), fc::assert_exception );
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
      cup.track_meta->json_metadata = "";
      FAIL( "with empty json metadata", cup );
      cup.track_meta->json_metadata = "{123: 123}";
      FAIL_WITH( "with invalid json metadata", cup, fc::parse_error_exception );
      cup.track_meta->json_metadata = "{\"id\": \"\200\"}";
      FAIL( "with non-utf8 json metadata", cup );
      cup.track_meta->json_metadata.reset();

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

      cup.comp_meta = content_metadata_publisher();
      cup.comp_meta->third_party_publishers = true;

      cup.new_management[0].percentage = 100;
      BOOST_TEST_MESSAGE( "--- Test success" );
      tx.operations.clear();
      tx.operations.push_back( cup );
      db.push_transaction( tx, database::skip_transaction_signatures  );
      }
      // --------- Verify update ------------
      {
      const content_object& song1 = db.get_content( "ipfs://abcdef1" );
      BOOST_CHECK( ! song1.comp_meta.third_party_publishers );
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

         auto last_update = db.head_block_time();
         try
         {
            for( uint32_t i = 0; i < MUSE_MAX_VOTE_CHANGES + 2; i++ )
            {
               generate_blocks( db.head_block_time() + MUSE_MIN_VOTE_INTERVAL_SEC + 1 );
               vop.weight++;
               tx.operations.clear();
               tx.operations.push_back( vop );
               db.push_transaction( tx, database::skip_transaction_signatures  );
               last_update = db.head_block_time();
            }
         }
         catch( fc::assert_exception& ex )
         {
             BOOST_CHECK_EQUAL( 1 + MUSE_MAX_VOTE_CHANGES + 1, vop.weight );
         }

         const content_object& song1 = db.get_content( "ipfs://abcdef1" );
         const auto& content_vote_idx = db.get_index_type< content_vote_index >().indices().get< by_content_voter >();
         const auto voted = content_vote_idx.find( std::make_tuple( song1.id, vici_id ) );
         BOOST_CHECK( voted != content_vote_idx.end() );
         BOOST_CHECK_EQUAL( vici_id, voted->voter );
         BOOST_CHECK_EQUAL( song1.id, voted->content );
         BOOST_CHECK_EQUAL( vop.weight - 1, voted->weight );
         BOOST_CHECK_EQUAL( MUSE_MAX_VOTE_CHANGES, voted->num_changes );
         BOOST_CHECK_EQUAL( last_update.sec_since_epoch(), voted->last_update.sec_since_epoch() );
      }

      BOOST_CHECK_EQUAL( 0, alice_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, suzy_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).balance.amount.value );

      BOOST_CHECK_EQUAL( 0, alice_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, suzy_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).mbd_balance.amount.value );

      BOOST_CHECK_EQUAL( 100000, alice_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, suzy_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, uhura_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, paula_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, penny_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, martha_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, muriel_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, colette_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, veronica_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, vici_id(db).vesting_shares.amount.value );

      BOOST_CHECK_EQUAL( 0, alice_id(db).curation_rewards.value );
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

      BOOST_CHECK_EQUAL( 0, alice_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, suzy_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).balance.amount.value );
      //BOOST_CHECK_EQUAL( 0, penny_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).balance.amount.value );
      //BOOST_CHECK_EQUAL( 0, veronica_id(db).balance.amount.value );
      //BOOST_CHECK_EQUAL( 0, vici_id(db).balance.amount.value );

      BOOST_CHECK_EQUAL( 0, alice_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, suzy_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, uhura_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, paula_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, penny_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, martha_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, muriel_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, colette_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, veronica_id(db).mbd_balance.amount.value );
      BOOST_CHECK_EQUAL( 0, vici_id(db).mbd_balance.amount.value );

      BOOST_CHECK_EQUAL( 100000, alice_id(db).vesting_shares.amount.value );
      //BOOST_CHECK_EQUAL( 100000, suzy_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, uhura_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, paula_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, penny_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, martha_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, muriel_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, colette_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, veronica_id(db).vesting_shares.amount.value );
      BOOST_CHECK_EQUAL( 100000, vici_id(db).vesting_shares.amount.value );

      BOOST_CHECK_EQUAL( 0, alice_id(db).curation_rewards.value );
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
      cop.track_meta.json_metadata = "{\"id\": 1}";
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
      cup.track_meta->json_metadata = "{\"id\": 1}";
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
      content_disable_operation cro;
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
      content_disable_operation cro;
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

BOOST_AUTO_TEST_CASE( balance_object_test )
{ try {
   const auto n_key = generate_private_key("n");
   const auto x_key = generate_private_key("x");

   // Intentionally overriding the fixture's db; I need to control genesis on this one.
   database db;
   fc::temp_directory td( graphene::utilities::temp_directory_path() );
   genesis_state_type genesis_state;
   {
   genesis_state_type::initial_balance_type balance;
   balance.owner = n_key.get_public_key();
   balance.asset_symbol = MUSE_SYMBOL;
   balance.amount = 1;
   genesis_state.initial_balances.push_back( balance );
   balance.owner = x_key.get_public_key();
   balance.amount = 10;
   genesis_state.initial_balances.push_back( balance );
   }
   fc::time_point_sec starting_time = genesis_state.initial_timestamp + 3000;

   genesis_state.initial_accounts.emplace_back("nina", n_key.get_public_key());
   genesis_state.initial_accounts.emplace_back("xana", x_key.get_public_key());

   genesis_state_type::initial_vesting_balance_type vest;
   vest.owner = account_id_type( 3 + MUSE_NUM_INIT_MINERS );
   vest.asset_symbol = MUSE_SYMBOL;
   vest.amount = 500;
   vest.begin_balance = vest.amount;
   vest.begin_timestamp = starting_time;
   vest.vesting_duration_seconds = 60;
   genesis_state.initial_vesting_balances.push_back(vest);
   vest.owner = account_id_type( 3 + MUSE_NUM_INIT_MINERS + 1);
   vest.begin_timestamp -= fc::seconds(30);
   vest.amount = 400;
   genesis_state.initial_vesting_balances.push_back(vest);

   auto _sign = [&]( signed_transaction& tx, const private_key_type& key )
   {  tx.sign( key, db.get_chain_id() );   };

   db.open( td.path(), genesis_state );
   const balance_object& balance = balance_id_type()(db);
   BOOST_CHECK_EQUAL(1, balance.balance.amount.value);
   BOOST_CHECK_EQUAL(10, balance_id_type(1)(db).balance.amount.value);

   const auto& account_n = db.get_account("nina");
   const auto& account_x = db.get_account("xana");
   ilog( "n: ${n_id}, x: ${x_id}", ("n_id",account_n.id)("x_id",account_x.id) );

   BOOST_CHECK_EQUAL(0, account_n.balance.amount.value);
   BOOST_CHECK_EQUAL(0, account_x.balance.amount.value);
   BOOST_CHECK_EQUAL(0, account_n.mbd_balance.amount.value);
   BOOST_CHECK_EQUAL(0, account_x.mbd_balance.amount.value);
   BOOST_CHECK_EQUAL(500, account_n.vesting_shares.amount.value);
   BOOST_CHECK_EQUAL(400, account_x.vesting_shares.amount.value);

   balance_claim_operation op;
   op.deposit_to_account = account_n.name;
   op.total_claimed = asset(1);
   op.balance_to_claim = balance_id_type(1);
   op.balance_owner_key = x_key.get_public_key();
   trx.operations = {op};
   _sign( trx, n_key );
   // Fail because I'm claiming from an address which hasn't signed
   MUSE_CHECK_THROW(db.push_transaction(trx), tx_missing_other_auth);
   trx.clear();
   op.balance_to_claim = balance_id_type();
   trx.operations = {op};
   _sign( trx, x_key );
   // Fail because I'm claiming from a wrong address
   MUSE_CHECK_THROW(db.push_transaction(trx), fc::assert_exception);
   trx.clear();
   op.balance_owner_key = n_key.get_public_key();
   trx.operations = {op};
   _sign( trx, x_key );
   // Fail because I'm claiming from an address which hasn't signed
   MUSE_CHECK_THROW(db.push_transaction(trx), tx_missing_other_auth);
   trx.clear();
   op.total_claimed = asset(2);
   trx.operations = {op};
   _sign( trx, n_key );
   // Fail because I'm claiming more than available
   MUSE_CHECK_THROW(db.push_transaction(trx), fc::assert_exception);
   trx.clear();
   op.total_claimed = asset(1);
   trx.operations = {op};
   _sign( trx, n_key );
   db.push_transaction(trx);

   BOOST_CHECK_EQUAL(account_n.balance.amount.value, 1);
   BOOST_CHECK(db.find_object(balance_id_type()) == nullptr);

   op.balance_to_claim = balance_id_type(1);
   op.balance_owner_key = x_key.get_public_key();
   trx.operations = {op};
   trx.signatures.clear();
   //_sign( trx, n_key );
   _sign( trx, x_key );
   db.push_transaction(trx);

   BOOST_CHECK_EQUAL(account_n.balance.amount.value, 2);
   BOOST_CHECK(db.find_object(balance_id_type(1)) != nullptr);

   validate_database();
   }
   FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE( friends_test )
{ try {
   ACTORS( (alice)(brenda)(charlene)(dora)(eve) );

   fund( "alice", 9000000 );
   fund( "brenda", 4000000 );
   fund( "charlene", 1000000 );
   fund( "dora", 810000 );
   fund( "eve", 640000 );

   const auto& dgpo = db.get_dynamic_global_properties();
   const auto& vest_to = [&]( const string& who, const uint64_t target )
   {
      const auto& acct = db.get_account( who );
      asset amount( target, VESTS_SYMBOL );
      amount = ( amount - acct.vesting_shares ) * dgpo.get_vesting_share_price();
      vest( who, amount.amount );

      BOOST_CHECK_EQUAL( acct.vesting_shares.amount.value, target );
   };

   vest_to( "alice", 900000000 );
   vest_to( "brenda", 400000000 );
   vest_to( "charlene", 100000000 );
   vest_to( "dora", 81000000 );
   vest_to( "eve", 64000000 );

   signed_transaction tx;
   tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );

   // --------- Make some friends ------------
   {
      friendship_operation fop;
      fop.who = "alice";
      fop.whom = "brenda";

      fop.who = "x";
      FAIL( "with bad account name", fop );

      fop.who = "bob";
      FAIL( "with non-existing account", fop );

      fop.who = "alice";
      fop.whom = "x";
      FAIL( "with bad other account name", fop );

      fop.whom = "bob";
      FAIL( "with non-existing other account", fop );

      fop.whom = "brenda";
      tx.operations.clear();
      tx.operations.push_back( fop );
      db.push_transaction( tx, database::skip_transaction_signatures  );

      fop.who = "dora";
      tx.operations.clear();
      tx.operations.push_back( fop );
      db.push_transaction( tx, database::skip_transaction_signatures  );

      fop.whom = "charlene";
      tx.operations.clear();
      tx.operations.push_back( fop );
      db.push_transaction( tx, database::skip_transaction_signatures  );

      fop.whom = "eve";
      tx.operations.clear();
      tx.operations.push_back( fop );
      db.push_transaction( tx, database::skip_transaction_signatures  );

      fop.who = "alice";
      tx.operations.clear();
      tx.operations.push_back( fop );
      db.push_transaction( tx, database::skip_transaction_signatures  );
   }

   BOOST_CHECK( alice.waiting.empty() );
   BOOST_CHECK( brenda.waiting.find( alice_id ) != brenda.waiting.end() );
   BOOST_CHECK( brenda.waiting.find( dora_id ) != brenda.waiting.end() );
   BOOST_CHECK_EQUAL( 2, brenda.waiting.size() );
   BOOST_CHECK( charlene.waiting.find( dora_id ) != charlene.waiting.end() );
   BOOST_CHECK_EQUAL( 1, charlene.waiting.size() );
   BOOST_CHECK( dora.waiting.empty() );
   BOOST_CHECK( eve.waiting.find( dora_id ) != eve.waiting.end() );
   BOOST_CHECK( eve.waiting.find( alice_id ) != eve.waiting.end() );
   BOOST_CHECK_EQUAL( 2, eve.waiting.size() );

   BOOST_CHECK( alice.friends.empty() );
   BOOST_CHECK( brenda.friends.empty() );
   BOOST_CHECK( charlene.friends.empty() );
   BOOST_CHECK( dora.friends.empty() );
   BOOST_CHECK( eve.friends.empty() );

   BOOST_CHECK( alice.second_level.empty() );
   BOOST_CHECK( brenda.second_level.empty() );
   BOOST_CHECK( charlene.second_level.empty() );
   BOOST_CHECK( dora.second_level.empty() );
   BOOST_CHECK( eve.second_level.empty() );

   {
      friendship_operation fop;
      fop.who = "brenda";
      fop.whom = "alice";
      tx.operations.clear();
      tx.operations.push_back( fop );
      db.push_transaction( tx, database::skip_transaction_signatures  );

      fop.whom = "dora";
      tx.operations.clear();
      tx.operations.push_back( fop );
      db.push_transaction( tx, database::skip_transaction_signatures  );

      fop.who = "charlene";
      tx.operations.clear();
      tx.operations.push_back( fop );
      db.push_transaction( tx, database::skip_transaction_signatures  );

      fop.who = "eve";
      tx.operations.clear();
      tx.operations.push_back( fop );
      db.push_transaction( tx, database::skip_transaction_signatures  );
   }

   BOOST_CHECK( alice.waiting.empty() );
   BOOST_CHECK( brenda.waiting.empty() );
   BOOST_CHECK( charlene.waiting.empty() );
   BOOST_CHECK( dora.waiting.empty() );
   BOOST_CHECK( eve.waiting.find( alice_id ) != eve.waiting.end() );
   BOOST_CHECK_EQUAL( 1, eve.waiting.size() );

   BOOST_CHECK( alice.friends.find( brenda_id ) != alice.friends.end() );
   BOOST_CHECK_EQUAL( 1, alice.friends.size() );
   BOOST_CHECK( brenda.friends.find( alice_id ) != brenda.friends.end() );
   BOOST_CHECK( brenda.friends.find( dora_id ) != brenda.friends.end() );
   BOOST_CHECK_EQUAL( 2, brenda.friends.size() );
   BOOST_CHECK( charlene.friends.find( dora_id ) != charlene.friends.end() );
   BOOST_CHECK_EQUAL( 1, charlene.friends.size() );
   BOOST_CHECK( dora.friends.find( brenda_id ) != dora.friends.end() );
   BOOST_CHECK( dora.friends.find( charlene_id ) != dora.friends.end() );
   BOOST_CHECK( dora.friends.find( eve_id ) != dora.friends.end() );
   BOOST_CHECK_EQUAL( 3, dora.friends.size() );
   BOOST_CHECK( eve.friends.find( dora_id ) != eve.friends.end() );
   BOOST_CHECK_EQUAL( 1, eve.friends.size() );

   BOOST_CHECK( alice.second_level.find( dora_id ) != alice.second_level.end() );
   BOOST_CHECK_EQUAL( 1, alice.second_level.size() );
   BOOST_CHECK( brenda.second_level.find( charlene_id ) != brenda.second_level.end() );
   BOOST_CHECK( brenda.second_level.find( eve_id ) != brenda.second_level.end() );
   BOOST_CHECK_EQUAL( 2, brenda.second_level.size() );
   BOOST_CHECK( charlene.second_level.find( brenda_id ) != charlene.second_level.end() );
   BOOST_CHECK( charlene.second_level.find( eve_id ) != charlene.second_level.end() );
   BOOST_CHECK_EQUAL( 2, charlene.second_level.size() );
   BOOST_CHECK( dora.second_level.find( alice_id ) != dora.second_level.end() );
   BOOST_CHECK_EQUAL( 1, dora.second_level.size() );
   BOOST_CHECK( eve.second_level.find( brenda_id ) != eve.second_level.end() );
   BOOST_CHECK( eve.second_level.find( charlene_id ) != eve.second_level.end() );
   BOOST_CHECK_EQUAL( 2, eve.second_level.size() );

   BOOST_CHECK_EQUAL( 30000 + 200 * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 90 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, alice.score );
   BOOST_CHECK_EQUAL( 20000 + (300 + 90) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + (100 + 80) * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, brenda.score );
   BOOST_CHECK_EQUAL( 10000 + 90 * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + (200 + 80) * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, charlene.score );
   BOOST_CHECK_EQUAL(  9000 + (200 + 100 + 80) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 300 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, dora.score );
   BOOST_CHECK_EQUAL(  8000 + 90 * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + (200 + 100) * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, eve.score );

   fund( "dora", 3000 );
   vest_to( "dora", 82810000 );

   BOOST_CHECK_EQUAL( 30000 + 200 * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 91 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, alice.score );
   BOOST_CHECK_EQUAL( 20000 + (300 + 91) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + (100 + 80) * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, brenda.score );
   BOOST_CHECK_EQUAL( 10000 + 91 * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + (200 + 80) * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, charlene.score );
   BOOST_CHECK_EQUAL(  9100 + (200 + 100 + 80) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 300 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, dora.score );
   BOOST_CHECK_EQUAL(  8000 + 91 * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + (200 + 100) * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, eve.score );

   {
      friendship_operation fop;
      fop.who = "eve";
      fop.whom = "alice";
      tx.operations.clear();
      tx.operations.push_back( fop );
      db.push_transaction( tx, database::skip_transaction_signatures  );
   }

   BOOST_CHECK( alice.waiting.empty() );
   BOOST_CHECK( brenda.waiting.empty() );
   BOOST_CHECK( charlene.waiting.empty() );
   BOOST_CHECK( dora.waiting.empty() );
   BOOST_CHECK( eve.waiting.empty() );

   BOOST_CHECK( alice.friends.find( brenda_id ) != alice.friends.end() );
   BOOST_CHECK( alice.friends.find( eve_id ) != alice.friends.end() );
   BOOST_CHECK_EQUAL( 2, alice.friends.size() );
   BOOST_CHECK( brenda.friends.find( alice_id ) != brenda.friends.end() );
   BOOST_CHECK( brenda.friends.find( dora_id ) != brenda.friends.end() );
   BOOST_CHECK_EQUAL( 2, brenda.friends.size() );
   BOOST_CHECK( charlene.friends.find( dora_id ) != charlene.friends.end() );
   BOOST_CHECK_EQUAL( 1, charlene.friends.size() );
   BOOST_CHECK( dora.friends.find( brenda_id ) != dora.friends.end() );
   BOOST_CHECK( dora.friends.find( charlene_id ) != dora.friends.end() );
   BOOST_CHECK( dora.friends.find( eve_id ) != dora.friends.end() );
   BOOST_CHECK_EQUAL( 3, dora.friends.size() );
   BOOST_CHECK( eve.friends.find( alice_id ) != eve.friends.end() );
   BOOST_CHECK( eve.friends.find( dora_id ) != eve.friends.end() );
   BOOST_CHECK_EQUAL( 2, eve.friends.size() );

   BOOST_CHECK( alice.second_level.find( dora_id ) != alice.second_level.end() );
   BOOST_CHECK_EQUAL( 1, alice.second_level.size() );
   BOOST_CHECK( brenda.second_level.find( charlene_id ) != brenda.second_level.end() );
   BOOST_CHECK( brenda.second_level.find( eve_id ) != brenda.second_level.end() );
   BOOST_CHECK_EQUAL( 2, brenda.second_level.size() );
   BOOST_CHECK( charlene.second_level.find( brenda_id ) != charlene.second_level.end() );
   BOOST_CHECK( charlene.second_level.find( eve_id ) != charlene.second_level.end() );
   BOOST_CHECK_EQUAL( 2, charlene.second_level.size() );
   BOOST_CHECK( dora.second_level.find( alice_id ) != dora.second_level.end() );
   BOOST_CHECK_EQUAL( 1, dora.second_level.size() );
   BOOST_CHECK( eve.second_level.find( brenda_id ) != eve.second_level.end() );
   BOOST_CHECK( eve.second_level.find( charlene_id ) != eve.second_level.end() );
   BOOST_CHECK_EQUAL( 2, eve.second_level.size() );

   BOOST_CHECK_EQUAL( 30000 + (200 + 80) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 91 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, alice.score );
   BOOST_CHECK_EQUAL( 20000 + (300 + 91) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + (100 + 80) * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, brenda.score );
   BOOST_CHECK_EQUAL( 10000 + 91 * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + (200 + 80) * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, charlene.score );
   BOOST_CHECK_EQUAL(  9100 + (200 + 100 + 80) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 300 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, dora.score );
   BOOST_CHECK_EQUAL(  8000 + (300 + 91) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + (200 + 100) * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, eve.score );

   // --------- Lose friends ------------
   {
      unfriend_operation ufo;
      ufo.who = "brenda";
      ufo.whom = "dora";

      ufo.who = "x";
      FAIL( "with bad account name", ufo );

      ufo.who = "bob";
      FAIL( "with non-existing account", ufo );

      ufo.who = "brenda";
      ufo.whom = "x";
      FAIL( "with bad other account name", ufo );

      ufo.whom = "bob";
      FAIL( "with non-existing other account", ufo );

      ufo.whom = "dora";
      tx.operations.clear();
      tx.operations.push_back( ufo );
      db.push_transaction( tx, database::skip_transaction_signatures  );
   }

   BOOST_CHECK( alice.waiting.empty() );
   BOOST_CHECK( brenda.waiting.empty() );
   BOOST_CHECK( charlene.waiting.empty() );
   BOOST_CHECK( dora.waiting.empty() );
   BOOST_CHECK( eve.waiting.empty() );

   BOOST_CHECK( alice.friends.find( brenda_id ) != alice.friends.end() );
   BOOST_CHECK( alice.friends.find( eve_id ) != alice.friends.end() );
   BOOST_CHECK_EQUAL( 2, alice.friends.size() );
   BOOST_CHECK( brenda.friends.find( alice_id ) != brenda.friends.end() );
   BOOST_CHECK_EQUAL( 1, brenda.friends.size() );
   BOOST_CHECK( charlene.friends.find( dora_id ) != charlene.friends.end() );
   BOOST_CHECK_EQUAL( 1, charlene.friends.size() );
   BOOST_CHECK( dora.friends.find( charlene_id ) != dora.friends.end() );
   BOOST_CHECK( dora.friends.find( eve_id ) != dora.friends.end() );
   BOOST_CHECK_EQUAL( 2, dora.friends.size() );
   BOOST_CHECK( eve.friends.find( alice_id ) != eve.friends.end() );
   BOOST_CHECK( eve.friends.find( dora_id ) != eve.friends.end() );
   BOOST_CHECK_EQUAL( 2, eve.friends.size() );

   BOOST_CHECK( alice.second_level.find( dora_id ) != alice.second_level.end() );
   BOOST_CHECK_EQUAL( 1, alice.second_level.size() );
   BOOST_CHECK( brenda.second_level.find( eve_id ) != brenda.second_level.end() );
   BOOST_CHECK_EQUAL( 1, brenda.second_level.size() );
   BOOST_CHECK( charlene.second_level.find( eve_id ) != charlene.second_level.end() );
   BOOST_CHECK_EQUAL( 1, charlene.second_level.size() );
   BOOST_CHECK( dora.second_level.find( alice_id ) != dora.second_level.end() );
   BOOST_CHECK_EQUAL( 1, dora.second_level.size() );
   BOOST_CHECK( eve.second_level.find( brenda_id ) != eve.second_level.end() );
   BOOST_CHECK( eve.second_level.find( charlene_id ) != eve.second_level.end() );
   BOOST_CHECK_EQUAL( 2, eve.second_level.size() );

   BOOST_CHECK_EQUAL( 30000 + (200 + 80) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 91 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, alice.score );
   BOOST_CHECK_EQUAL( 20000 + 300 * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 80 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, brenda.score );
   BOOST_CHECK_EQUAL( 10000 + 91 * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 80 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, charlene.score );
   BOOST_CHECK_EQUAL(  9100 + (100 + 80) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 300 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, dora.score );
   BOOST_CHECK_EQUAL(  8000 + (300 + 91) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + (200 + 100) * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, eve.score );

   {
      withdraw_vesting_operation op;
      op.account = "alice";
      op.vesting_shares = asset( 767000000, VESTS_SYMBOL );
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_CHECK_EQUAL( 59000000, alice.vesting_withdraw_rate.amount.value );
   }

   auto next_withdrawal = db.head_block_time() + MUSE_VESTING_WITHDRAW_INTERVAL_SECONDS;
   generate_blocks( next_withdrawal - ( MUSE_BLOCK_INTERVAL / 2 ), true);
   generate_block();

   BOOST_CHECK_EQUAL( 29000 + (200 + 80) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 91 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, alice_id(db).score );
   BOOST_CHECK_EQUAL( 20000 + 290 * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 80 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, brenda_id(db).score );
   BOOST_CHECK_EQUAL( 10000 + 91 * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 80 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, charlene_id(db).score );
   BOOST_CHECK_EQUAL(  9100 + (100 + 80) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + 290 * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, dora_id(db).score );
   BOOST_CHECK_EQUAL(  8000 + (290 + 91) * MUSE_1ST_LEVEL_SCORING_PERCENTAGE + (200 + 100) * MUSE_2ST_LEVEL_SCORING_PERCENTAGE, eve_id(db).score );

   validate_database();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( disable_test )
{ try {
   generate_blocks( time_point_sec( MUSE_HARDFORK_0_1_TIME ) );
   BOOST_CHECK( db.has_hardfork( MUSE_HARDFORK_0_1 ) );

   BOOST_TEST_MESSAGE( "Testing: streaming platform contract disable" );

   ACTORS( (alice)(suzy)(uhura)(paula)(martha)(colette)(veronica) );

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
      db.push_transaction( tx, database::skip_transaction_signatures );
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
      db.push_transaction( tx, database::skip_transaction_signatures  );
   }

   // --------- Content removal ------------
   {
      content_disable_operation cro;
      cro.url = "ipfs://abcdef1";

      cro.url = "http://abcdef1";
      FAIL( "with bad url protocol", cro );
      cro.url = "";
      FAIL( "with empty url", cro );
      cro.url = "ipfs://1234567890";
      for( int i = 0; i < MUSE_MAX_URL_LENGTH / 10; i++ )
          cro.url += "1234567890";
      FAIL( "with too long url", cro );

      cro.url = "ipfs://abcdef1";
      tx.operations.clear();
      tx.operations.push_back( cro );
      db.push_transaction( tx, database::skip_transaction_signatures  );

      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION - 1 );
      FAIL( "double disable", cro );
   }

   // --------- Approve content ------------
   {
       content_approve_operation cao;
       cao.approver = "alice";
       cao.url = "ipfs://abcdef1";
       FAIL( "approve after disable", cao );
   }

   // --------- Content update ------------
   {
      content_update_operation cup;
      cup.side = content_update_operation::side_t::master;
      cup.url = "ipfs://abcdef1";
      cup.new_publishers_share = 1;
      cup.album_meta = content_metadata_album_master();
      cup.album_meta->album_title = "Simple test album";
      cup.track_meta = content_metadata_track_master();
      cup.track_meta->track_title = "Simple test track";
      FAIL( "update after disable", cup );
   }

   // --------- Vote ------------
   {
      vote_operation vop;
      vop.voter = "veronica";
      vop.url = "ipfs://abcdef1";
      vop.weight = 1;
      FAIL( "vote after disable", vop );
   }

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
