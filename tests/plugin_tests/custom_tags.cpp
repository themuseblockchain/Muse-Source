/*
 * Copyright (c) 2018 Peertracks, Inc., and contributors.
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

#include <muse/app/api_context.hpp>

#include <muse/chain/protocol/base_operations.hpp>

#include <muse/custom_tags/custom_tags_api.hpp>

#include "../common/database_fixture.hpp"

using namespace muse::chain;
using namespace muse::chain::test;

BOOST_FIXTURE_TEST_SUITE( custom_tags, clean_database_fixture );

BOOST_AUTO_TEST_CASE( simple_test )
{ try {
   ACTORS( (alice)(brenda)(tamara)(tess) );

   generate_block();

   const auto& idx = db.get_index_type<muse::custom_tags::custom_tags_index>().indices().get<muse::custom_tags::by_tagger>();

   trx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );

   custom_json_operation cop;
   cop.required_basic_auths.insert( "tess" );

   cop.json = "1"; // not an object
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "\"hello\""; // not an object
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "[]"; // not an object
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "true"; // not an object
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{}"; // missing label
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":{}, \"set\":[\"alice\"]}"; // label is not a string
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":1, \"set\":[\"alice\"]}"; // label is not a string
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":false, \"set\":[\"alice\"]}"; // label is not a string
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":\"test\"}"; // missing set/add/remove
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":\"test\", \"set\":{}}"; // set is not an array
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":\"test\", \"set\":1}"; // set is not an array
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":\"test\", \"set\":\"alice\"}"; // set is not an array
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":\"test\", \"set\":[]}"; // set is empty
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"set\":[\"alice\"]}"; // missing label
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":\"test\", \"set\":[\"alice\"], \"add\":[\"brenda\"]}"; // set and add
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":\"test\", \"set\":[\"alice\"], \"remove\":[\"brenda\"]}"; // set and remove
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":\"x\",\"set\":[\"zarah\"]}"; // unknown user
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":\"x\",\"add\":[\"zarah\"]}"; // unknown user
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.json = "{\"label\":\"x\",\"remove\":[\"zarah\"]}"; // unknown user
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.required_auths.insert( "tess" ); // too many taggers
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.required_auths.insert( "tamara" ); // too many taggers
   cop.required_basic_auths.clear();
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.required_basic_auths.insert( "tamara" ); // too many taggers
   cop.required_basic_auths.insert( "tess" );
   cop.required_auths.clear();
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK( idx.empty() );

   cop.required_basic_auths.clear();
   cop.required_basic_auths.insert( "tess" );
   cop.json = "{\"label\":\"x\",\"set\":[\"alice\"]}"; // works
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK_EQUAL( 1, idx.size() );

   cop.required_basic_auths.clear();
   cop.required_auths.insert( "tamara" );
   cop.json = "{\"label\":\"x\",\"add\":[\"brenda\"]}"; // works
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK_EQUAL( 2, idx.size() );

   cop.required_auths.clear();
   cop.required_basic_auths.insert( "tess" );
   cop.json = "{\"label\":\"x\",\"remove\":[\"alice\"]}"; // works
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK_EQUAL( 1, idx.size() );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( api_test )
{ try {
   ACTORS( (alice)(brenda)(charlene)(dora)(tamara)(tess) );

   generate_block();

   const auto& idx = db.get_index_type<muse::custom_tags::custom_tags_index>().indices().get<muse::custom_tags::by_tagger>();
   custom_json_operation cop;

   cop.required_auths.insert( "tamara" );
   cop.json = "{\"label\":\"abc\",\"set\":[\"alice\",\"brenda\"]}";
   trx.operations.push_back( cop );
   cop.json = "{\"label\":\"test\",\"add\":[\"charlene\",\"brenda\"]}";
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK_EQUAL( 4, idx.size() );

   cop.required_basic_auths.insert( "tess" );
   cop.required_auths.clear();
   cop.json = "{\"label\":\"abc\",\"set\":[\"alice\",\"brenda\"]}";
   trx.operations.push_back( cop );
   cop.json = "{\"label\":\"bad girl\",\"add\":[\"dora\"]}";
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK_EQUAL( 7, idx.size() );


   //muse::custom_tags::custom_tags_api& api = *reinterpret_cast<muse::custom_tags::custom_tags_api*>(app.create_api_by_name( muse::app::api_context( app, "custom_tags", std::weak_ptr<muse::app::api_session_data>(std::shared_ptr<muse::app::api_session_data>(nullptr)) ) )->as<fc::api<muse::custom_tags::custom_tags_api>>().get_handle());
   muse::custom_tags::custom_tags_api api( muse::app::api_context( app, "custom_tags", std::weak_ptr<muse::app::api_session_data>(std::shared_ptr<muse::app::api_session_data>(nullptr)) ) );

   BOOST_CHECK( api.is_tagged( "alice", "tamara", "abc" ) );
   BOOST_CHECK( api.is_tagged( "brenda", "tamara", "abc" ) );
   BOOST_CHECK( api.is_tagged( "charlene", "tamara", "test" ) );
   BOOST_CHECK( api.is_tagged( "brenda", "tamara", "test" ) );
   BOOST_CHECK( !api.is_tagged( "charlene", "tamara", "abc" ) );
   BOOST_CHECK( !api.is_tagged( "alice", "tamara", "test" ) );

   BOOST_CHECK( api.is_tagged( "alice", "tess", "abc" ) );
   BOOST_CHECK( api.is_tagged( "brenda", "tess", "abc" ) );
   BOOST_CHECK( api.is_tagged( "dora", "tess", "bad girl" ) );
   BOOST_CHECK( !api.is_tagged( "dora", "tess", "abc" ) );
   BOOST_CHECK( !api.is_tagged( "alice", "tess", "bad girl" ) );
   BOOST_CHECK( !api.is_tagged( "brenda", "tess", "bad girl" ) );


   auto list = api.list_tags_from( "tamara", "", "", "" );
   BOOST_REQUIRE_EQUAL( 4, list.size() );
   BOOST_CHECK_EQUAL( "abc", list[0].tag );
   BOOST_CHECK_EQUAL( "alice", list[0].user );
   BOOST_CHECK_EQUAL( "abc", list[1].tag );
   BOOST_CHECK_EQUAL( "brenda", list[1].user );
   BOOST_CHECK_EQUAL( "test", list[2].tag );
   BOOST_CHECK_EQUAL( "brenda", list[2].user );
   BOOST_CHECK_EQUAL( "test", list[3].tag );
   BOOST_CHECK_EQUAL( "charlene", list[3].user );

   list = api.list_tags_from( "tamara", "test", "", "" );
   BOOST_REQUIRE_EQUAL( 2, list.size() );
   BOOST_CHECK_EQUAL( "test", list[0].tag );
   BOOST_CHECK_EQUAL( "brenda", list[0].user );
   BOOST_CHECK_EQUAL( "test", list[1].tag );
   BOOST_CHECK_EQUAL( "charlene", list[1].user );

   list = api.list_tags_from( "tamara", "", "abc", "brenda" );
   BOOST_REQUIRE_EQUAL( 3, list.size() );

   list = api.list_tags_from( "tamara", "", "zak", "" );
   BOOST_CHECK( list.empty() );

   list = api.list_tags_from( "alice", "", "", "" );
   BOOST_CHECK( list.empty() );


   list = api.list_tags_on( "alice", "", "", "" );
   BOOST_REQUIRE_EQUAL( 2, list.size() );
   BOOST_CHECK_EQUAL( "abc", list[0].tag );
   BOOST_CHECK_EQUAL( "tamara", list[0].user );
   BOOST_CHECK_EQUAL( "abc", list[1].tag );
   BOOST_CHECK_EQUAL( "tess", list[1].user );

   list = api.list_tags_on( "brenda", "bad girl", "", "" );
   BOOST_CHECK( list.empty() );

   list = api.list_tags_on( "brenda", "", "bad girl", "" );
   BOOST_CHECK_EQUAL( 1, list.size() );
   BOOST_CHECK_EQUAL( "test", list[0].tag );
   BOOST_CHECK_EQUAL( "tamara", list[0].user );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
