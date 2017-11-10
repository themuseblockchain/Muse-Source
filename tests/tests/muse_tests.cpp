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

BOOST_AUTO_TEST_CASE( streamingplatform_test )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: streaming platform contract" );

      muse::app::database_api dbapi(db);

      ACTORS( (suzy)(uhura)(paula)(martha)(colette) );

      signed_transaction tx;
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );

      // --------- Create streaming platform ------------

      streaming_platform_update_operation spuo;
      spuo.fee = asset( MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE, MUSE_SYMBOL );
      spuo.owner = "suzy";
      spuo.url = "http://www.google.de";
      tx.operations.push_back( spuo );

      FAIL( "when insufficient funds for fee", spuo );

      fund( "suzy", 10 * MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE );

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

      // --------- Look up streaming platforms ------------

      set<string> sps = dbapi.lookup_streaming_platform_accounts("x", 5);
      BOOST_CHECK( sps.empty() );

      sps = dbapi.lookup_streaming_platform_accounts("", 5);
      BOOST_CHECK_EQUAL( 1, sps.size() );
      BOOST_CHECK( sps.find("suzy") != sps.end() );

      // --------- Create content ------------

      fund( "uhura", 1000000 );

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

      // --------- Publish playtime ------------

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

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
