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

#include <muse/app/database_api.hpp>
#include <muse/chain/protocol/operations.hpp>

#include "../common/database_fixture.hpp"

using namespace muse::chain;
using namespace graphene::db;

BOOST_FIXTURE_TEST_SUITE( api_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( get_accounts_test )
{ try {
   muse::app::database_api db_api( db );

   ACTORS( (alice)(brenda)(charlene)(paula)(penny)(uhura) );
   fund( "alice" );
   vest( "alice", 50000 );
   fund( "brenda" );
   fund( "charlene" );

   generate_block();
   trx.clear();
   trx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
   const auto& pidx = db.get_index_type<proposal_index>().indices().get<by_id>();

   { // active authority
      transfer_operation top;
      top.from = "alice";
      top.to   = "brenda";
      top.amount = asset( 10000, MUSE_SYMBOL );

      proposal_create_operation pco;
      pco.proposed_ops.emplace_back( top );
      pco.expiration_time = db.head_block_time() + fc::days(1);

      trx.operations.push_back( pco );
      PUSH_TX( db, trx );
      trx.clear();
   }
   BOOST_REQUIRE_EQUAL( 1, pidx.size() );
   proposal_id_type pid1 = pidx.begin()->id;

   {
      vector<muse::app::extended_account> accounts = db_api.get_accounts( { "alice", "brenda", "charlene" } );
      muse::app::extended_account alice_account = accounts[0];
      muse::app::extended_account brenda_account = accounts[1];
      muse::app::extended_account charlene_account = accounts[2];

      BOOST_CHECK_EQUAL( "alice", alice_account.name );
      BOOST_CHECK_EQUAL( 50100, alice_account.muse_power.amount.value );
      BOOST_CHECK_EQUAL( MUSE_SYMBOL, alice_account.muse_power.asset_id );
      BOOST_CHECK_EQUAL( 1, alice_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid1.instance.value, alice_account.proposals[0].id.instance() );

      BOOST_CHECK_EQUAL( "brenda", brenda_account.name );
      BOOST_CHECK_EQUAL( 100, brenda_account.muse_power.amount.value );
      BOOST_CHECK_EQUAL( MUSE_SYMBOL, brenda_account.muse_power.asset_id );
      BOOST_CHECK_EQUAL( 0, brenda_account.proposals.size() );

      BOOST_CHECK_EQUAL( "charlene", charlene_account.name );
      BOOST_CHECK_EQUAL( 100, charlene_account.muse_power.amount.value );
      BOOST_CHECK_EQUAL( MUSE_SYMBOL, charlene_account.muse_power.asset_id );
      BOOST_CHECK_EQUAL( 0, charlene_account.proposals.size() );
   }

   { // basic authority
      proposal_create_operation pco;

      friendship_operation fop;
      fop.who  = "brenda";
      fop.whom = "charlene";
      pco.proposed_ops.emplace_back( fop );

      std::swap( fop.who, fop.whom );
      pco.proposed_ops.emplace_back( fop );

      pco.expiration_time = db.head_block_time() + fc::days(1);
      trx.operations.push_back( pco );
      PUSH_TX( db, trx );
      trx.clear();
   }
   BOOST_REQUIRE_EQUAL( 2, pidx.size() );
   proposal_id_type pid2 = std::next(pidx.begin())->id;

   {
      vector<muse::app::extended_account> accounts = db_api.get_accounts( { "alice", "brenda", "charlene" } );
      muse::app::extended_account alice_account = accounts[0];
      muse::app::extended_account brenda_account = accounts[1];
      muse::app::extended_account charlene_account = accounts[2];

      BOOST_CHECK_EQUAL( "alice", alice_account.name );
      BOOST_CHECK_EQUAL( 1, alice_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid1.instance.value, alice_account.proposals[0].id.instance() );

      BOOST_CHECK_EQUAL( "brenda", brenda_account.name );
      BOOST_CHECK_EQUAL( 1, brenda_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, brenda_account.proposals[0].id.instance() );

      BOOST_CHECK_EQUAL( "charlene", charlene_account.name );
      BOOST_CHECK_EQUAL( 1, charlene_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, charlene_account.proposals[0].id.instance() );
   }

   { // owner authority
      account_update_operation aop;
      aop.account = "brenda";
      aop.owner = authority( 1, "alice", 1 );

      proposal_create_operation pco;
      pco.proposed_ops.emplace_back( aop );
      pco.expiration_time = db.head_block_time() + fc::days(1);

      trx.operations.push_back( pco );
      PUSH_TX( db, trx );
      trx.clear();
   }
   BOOST_REQUIRE_EQUAL( 3, pidx.size() );
   proposal_id_type pid3 = std::next(pidx.begin(), 2)->id;

   {
      vector<muse::app::extended_account> accounts = db_api.get_accounts( { "alice", "brenda", "charlene" } );
      muse::app::extended_account alice_account = accounts[0];
      muse::app::extended_account brenda_account = accounts[1];
      muse::app::extended_account charlene_account = accounts[2];

      BOOST_CHECK_EQUAL( "alice", alice_account.name );
      BOOST_CHECK_EQUAL( 1, alice_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid1.instance.value, alice_account.proposals[0].id.instance() );

      BOOST_CHECK_EQUAL( "brenda", brenda_account.name );
      BOOST_CHECK_EQUAL( 2, brenda_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, brenda_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid3.instance.value, brenda_account.proposals[1].id.instance() );

      BOOST_CHECK_EQUAL( "charlene", charlene_account.name );
      BOOST_CHECK_EQUAL( 1, charlene_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, charlene_account.proposals[0].id.instance() );
   }

   const string url = "ipfs://abcdef1";
   {
      content_operation cop;
      cop.uploader = "uhura";
      cop.url = url;
      cop.album_meta.album_title = "First test song";
      cop.track_meta.track_title = "First test song";
      cop.comp_meta.third_party_publishers = true;
      distribution dist;
      dist.payee = "paula";
      dist.bp = MUSE_100_PERCENT;
      cop.distributions.push_back( dist );
      dist.payee = "penny";
      cop.distributions_comp = vector<distribution>();
      cop.distributions_comp->push_back( dist );
      management_vote mgmt;
      mgmt.voter = "alice";
      mgmt.percentage = 100;
      cop.management.push_back( mgmt );
      cop.management_threshold = 100;
      mgmt.voter = "brenda";
      mgmt.percentage = 50;
      cop.management_comp = vector<management_vote>();
      cop.management_comp->push_back(mgmt);
      mgmt.voter = "charlene";
      cop.management_comp->push_back(mgmt);
      cop.management_threshold_comp = 100;
      cop.playing_reward = 10;
      cop.publishers_share = 0;

      trx.operations.push_back( cop );
      sign( trx, uhura_private_key );
      PUSH_TX( db, trx );
      trx.clear();
   }

   { // management authority
      content_disable_operation cdop;
      cdop.url = url;

      proposal_create_operation pco;
      pco.proposed_ops.emplace_back( cdop );
      pco.expiration_time = db.head_block_time() + fc::days(1);

      trx.operations.push_back( pco );
      PUSH_TX( db, trx );
      trx.clear();
   }
   BOOST_REQUIRE_EQUAL( 4, pidx.size() );
   proposal_id_type pid4 = std::next(pidx.begin(), 3)->id;

   {
      vector<muse::app::extended_account> accounts = db_api.get_accounts( { "alice", "brenda", "charlene" } );
      muse::app::extended_account alice_account = accounts[0];
      muse::app::extended_account brenda_account = accounts[1];
      muse::app::extended_account charlene_account = accounts[2];

      BOOST_CHECK_EQUAL( "alice", alice_account.name );
      BOOST_CHECK_EQUAL( 2, alice_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid1.instance.value, alice_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid4.instance.value, alice_account.proposals[1].id.instance() );

      BOOST_CHECK_EQUAL( "brenda", brenda_account.name );
      BOOST_CHECK_EQUAL( 2, brenda_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, brenda_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid3.instance.value, brenda_account.proposals[1].id.instance() );

      BOOST_CHECK_EQUAL( "charlene", charlene_account.name );
      BOOST_CHECK_EQUAL( 1, charlene_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, charlene_account.proposals[0].id.instance() );
   }

   { // comp authority
      content_update_operation cup;
      cup.url = url;
      cup.side = content_update_operation::side_t::publisher;
      cup.new_threshold = 60;

      proposal_create_operation pco;
      pco.proposed_ops.emplace_back( cup );
      pco.expiration_time = db.head_block_time() + fc::days(1);

      trx.operations.push_back( pco );
      PUSH_TX( db, trx );
      trx.clear();
   }
   BOOST_REQUIRE_EQUAL( 5, pidx.size() );
   proposal_id_type pid5 = std::next(pidx.begin(), 4)->id;

   {
      vector<muse::app::extended_account> accounts = db_api.get_accounts( { "alice", "brenda", "charlene" } );
      muse::app::extended_account alice_account = accounts[0];
      muse::app::extended_account brenda_account = accounts[1];
      muse::app::extended_account charlene_account = accounts[2];

      BOOST_CHECK_EQUAL( "alice", alice_account.name );
      BOOST_CHECK_EQUAL( 2, alice_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid1.instance.value, alice_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid4.instance.value, alice_account.proposals[1].id.instance() );

      BOOST_CHECK_EQUAL( "brenda", brenda_account.name );
      BOOST_CHECK_EQUAL( 3, brenda_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, brenda_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid3.instance.value, brenda_account.proposals[1].id.instance() );
      BOOST_CHECK_EQUAL( pid5.instance.value, brenda_account.proposals[2].id.instance() );

      BOOST_CHECK_EQUAL( "charlene", charlene_account.name );
      BOOST_CHECK_EQUAL( 2, charlene_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, charlene_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid5.instance.value, charlene_account.proposals[1].id.instance() );
   }

   { // Brenda approves 2
      proposal_update_operation pup;
      pup.proposal = pid2;
      pup.active_approvals_to_add.insert( "brenda" );
      trx.operations.push_back( pup );
      sign( trx, brenda_private_key );
      PUSH_TX( db, trx );
      trx.clear();
   }
   BOOST_CHECK_EQUAL( 5, pidx.size() );

   {
      vector<muse::app::extended_account> accounts = db_api.get_accounts( { "alice", "brenda", "charlene" } );
      muse::app::extended_account alice_account = accounts[0];
      muse::app::extended_account brenda_account = accounts[1];
      muse::app::extended_account charlene_account = accounts[2];

      BOOST_CHECK_EQUAL( "alice", alice_account.name );
      BOOST_CHECK_EQUAL( 2, alice_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid1.instance.value, alice_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid4.instance.value, alice_account.proposals[1].id.instance() );

      BOOST_CHECK_EQUAL( "brenda", brenda_account.name );
      BOOST_CHECK_EQUAL( 3, brenda_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, brenda_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid3.instance.value, brenda_account.proposals[1].id.instance() );
      BOOST_CHECK_EQUAL( pid5.instance.value, brenda_account.proposals[2].id.instance() );
      const proposal_object& prop2 = db.get<proposal_object>(pid2);
      BOOST_REQUIRE_EQUAL( 1, prop2.available_active_approvals.size() );
      BOOST_CHECK_EQUAL( "brenda", *prop2.available_active_approvals.begin() );

      BOOST_CHECK_EQUAL( "charlene", charlene_account.name );
      BOOST_CHECK_EQUAL( 2, charlene_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, charlene_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid5.instance.value, charlene_account.proposals[1].id.instance() );
   }

   { // Alice approves 1
      proposal_update_operation pup;
      pup.proposal = pid1;
      pup.active_approvals_to_add.insert( "alice" );
      trx.operations.push_back( pup );
      sign( trx, alice_private_key );
      PUSH_TX( db, trx );
      trx.clear();
   }
   BOOST_CHECK_EQUAL( 4, pidx.size() );
   BOOST_CHECK_THROW( db.get<proposal_object>(pid1), fc::assert_exception );

   {
      vector<muse::app::extended_account> accounts = db_api.get_accounts( { "alice", "brenda", "charlene" } );
      muse::app::extended_account alice_account = accounts[0];
      muse::app::extended_account brenda_account = accounts[1];
      muse::app::extended_account charlene_account = accounts[2];

      BOOST_CHECK_EQUAL( "alice", alice_account.name );
      BOOST_CHECK_EQUAL( 1, alice_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid4.instance.value, alice_account.proposals[0].id.instance() );

      BOOST_CHECK_EQUAL( "brenda", brenda_account.name );
      BOOST_CHECK_EQUAL( 3, brenda_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, brenda_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid3.instance.value, brenda_account.proposals[1].id.instance() );
      BOOST_CHECK_EQUAL( pid5.instance.value, brenda_account.proposals[2].id.instance() );

      BOOST_CHECK_EQUAL( "charlene", charlene_account.name );
      BOOST_CHECK_EQUAL( 2, charlene_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, charlene_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid5.instance.value, charlene_account.proposals[1].id.instance() );
   }

   { // Brenda approves 3
      proposal_update_operation pup;
      pup.proposal = pid3;
      pup.owner_approvals_to_add.insert( "brenda" );
      trx.operations.push_back( pup );

      sign( trx, brenda_private_key );
      PUSH_TX( db, trx );
      trx.clear();
   }
   BOOST_CHECK_EQUAL( 3, pidx.size() );
   BOOST_CHECK_THROW( db.get<proposal_object>(pid3), fc::assert_exception );

   {
      vector<muse::app::extended_account> accounts = db_api.get_accounts( { "alice", "brenda", "charlene" } );
      muse::app::extended_account alice_account = accounts[0];
      muse::app::extended_account brenda_account = accounts[1];
      muse::app::extended_account charlene_account = accounts[2];

      BOOST_CHECK_EQUAL( "alice", alice_account.name );
      BOOST_CHECK_EQUAL( 1, alice_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid4.instance.value, alice_account.proposals[0].id.instance() );

      BOOST_CHECK_EQUAL( "brenda", brenda_account.name );
      BOOST_CHECK_EQUAL( 2, brenda_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, brenda_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid5.instance.value, brenda_account.proposals[1].id.instance() );

      BOOST_CHECK_EQUAL( "charlene", charlene_account.name );
      BOOST_CHECK_EQUAL( 2, charlene_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid2.instance.value, charlene_account.proposals[0].id.instance() );
      BOOST_CHECK_EQUAL( pid5.instance.value, charlene_account.proposals[1].id.instance() );
   }

   { // Charlene approves 2 and 5
      proposal_update_operation pup;
      pup.proposal = pid2;
      pup.active_approvals_to_add.insert( "charlene" );
      trx.operations.push_back( pup );

      pup.proposal = pid5;
      trx.operations.push_back( pup );

      sign( trx, charlene_private_key );
      PUSH_TX( db, trx );
      trx.clear();
   }
   BOOST_CHECK_EQUAL( 2, pidx.size() );

   {
      vector<muse::app::extended_account> accounts = db_api.get_accounts( { "alice", "brenda", "charlene" } );
      muse::app::extended_account alice_account = accounts[0];
      muse::app::extended_account brenda_account = accounts[1];
      muse::app::extended_account charlene_account = accounts[2];

      BOOST_CHECK_EQUAL( "alice", alice_account.name );
      BOOST_CHECK_EQUAL( 1, alice_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid4.instance.value, alice_account.proposals[0].id.instance() );

      BOOST_CHECK_EQUAL( "brenda", brenda_account.name );
      BOOST_CHECK_EQUAL( 1, brenda_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid5.instance.value, brenda_account.proposals[0].id.instance() );

      BOOST_CHECK_EQUAL( "charlene", charlene_account.name );
      BOOST_CHECK_EQUAL( 1, charlene_account.proposals.size() );
      BOOST_CHECK_EQUAL( pid5.instance.value, charlene_account.proposals[0].id.instance() );
   }

   { // Alice approves 5 on behalf of Brenda and 4 for herself
      proposal_update_operation pup;
      pup.proposal = pid5;
      pup.active_approvals_to_add.insert( "brenda" );
      trx.operations.push_back( pup );

      pup.proposal = pid4;
      pup.active_approvals_to_add.clear();
      pup.active_approvals_to_add.insert( "alice" );
      trx.operations.push_back( pup );

      sign( trx, alice_private_key );
      PUSH_TX( db, trx );
      trx.clear();
   }
   BOOST_CHECK_EQUAL( 0, pidx.size() );

   {
      vector<muse::app::extended_account> accounts = db_api.get_accounts( { "alice", "brenda", "charlene" } );
      muse::app::extended_account alice_account = accounts[0];
      muse::app::extended_account brenda_account = accounts[1];
      muse::app::extended_account charlene_account = accounts[2];

      BOOST_CHECK_EQUAL( "alice", alice_account.name );
      BOOST_CHECK_EQUAL( 0, alice_account.proposals.size() );

      BOOST_CHECK_EQUAL( "brenda", brenda_account.name );
      BOOST_CHECK_EQUAL( 0, brenda_account.proposals.size() );

      BOOST_CHECK_EQUAL( "charlene", charlene_account.name );
      BOOST_CHECK_EQUAL( 0, charlene_account.proposals.size() );
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
