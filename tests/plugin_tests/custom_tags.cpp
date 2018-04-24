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

#include <muse/chain/protocol/base_operations.hpp>

#include <muse/custom_tags/custom_tags_api.hpp>

#include "../common/database_fixture.hpp"

using namespace muse::chain;
using namespace muse::chain::test;

BOOST_FIXTURE_TEST_SUITE( custom_tags, clean_database_fixture );

BOOST_AUTO_TEST_CASE( simple_test )
{ try {
   ACTORS( (alice)(brenda)(tess) );

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

   cop.json = "{\"label\":\"x\",\"set\":[\"alice\"]}"; // works
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK_EQUAL( 1, idx.size() );

   cop.json = "{\"label\":\"x\",\"add\":[\"brenda\"]}"; // works
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK_EQUAL( 2, idx.size() );

   cop.json = "{\"label\":\"x\",\"remove\":[\"alice\"]}"; // works
   trx.operations.push_back( cop );
   PUSH_TX( db, trx, database::skip_transaction_signatures );
   trx.clear();
   BOOST_CHECK_EQUAL( 1, idx.size() );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
