/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
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

#include <muse/chain/database.hpp>
#include <muse/chain/protocol/protocol.hpp>
#include <muse/chain/exceptions.hpp>

#include <muse/chain/account_object.hpp>
#include <muse/chain/asset_object.hpp>
#include <muse/chain/content_object.hpp>
#include <muse/chain/proposal_object.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace muse::chain;
using namespace muse::chain::test;

BOOST_FIXTURE_TEST_SUITE( authority_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( invalid_authorities )
{ try {

   ACTORS( (alice) );
   fund( "alice" );

   private_key_type brenda_private_key = generate_private_key( "brenda" );
   account_create_operation aco;
   aco.creator = "alice";
   aco.new_account_name = "brenda";
   aco.fee = asset( 1000 );
   aco.owner.key_auths[brenda_private_key.get_public_key()] = 1;
   aco.owner.weight_threshold = 1;
   aco.active.key_auths[brenda_private_key.get_public_key()] = 1;
   aco.active.weight_threshold = 1;
   aco.basic.key_auths[brenda_private_key.get_public_key()] = 1;
   aco.basic.weight_threshold = 1;
   aco.memo_key = brenda_private_key.get_public_key();

   aco.owner.account_auths["Hello world!"] = 1;
   trx.operations.push_back(aco);
   sign(trx, alice_private_key);
   BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
   trx.clear();

   proposal_create_operation pop;
   pop.proposed_ops.emplace_back( aco );
   pop.expiration_time = db.head_block_time() + fc::minutes(1);
   trx.operations.push_back( pop );
   BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
   pop.proposed_ops.clear();
   trx.clear();

   aco.owner.account_auths.clear();
   aco.active.account_auths["Hello world!"] = 1;
   trx.operations.push_back(aco);
   sign(trx, alice_private_key);
   BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
   trx.clear();

   pop.proposed_ops.emplace_back( aco );
   trx.operations.push_back( pop );
   BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
   pop.proposed_ops.clear();
   trx.clear();

   aco.active.account_auths.clear();
   aco.basic.account_auths["Hello world!"] = 1;
   trx.operations.push_back(aco);
   sign(trx, alice_private_key);
   BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
   trx.clear();

   pop.proposed_ops.emplace_back( aco );
   trx.operations.push_back( pop );
   BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
   pop.proposed_ops.clear();
   trx.clear();

   {
      account_update_operation aup;
      aup.account = "alice";
      aup.owner = authority( 1, "Hello world!", 1 );
      trx.operations.push_back(aup);
      sign(trx, alice_private_key);
      BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
      trx.clear();

      pop.proposed_ops.emplace_back( aup );
      trx.operations.push_back( pop );
      BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
      pop.proposed_ops.clear();
      trx.clear();
   }

   {
      account_update_operation aup;
      aup.account = "alice";
      aup.active = authority( 1, "Hello world!", 1 );
      trx.operations.push_back(aup);
      sign(trx, alice_private_key);
      BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
      trx.clear();

      pop.proposed_ops.emplace_back( aup );
      trx.operations.push_back( pop );
      BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
      pop.proposed_ops.clear();
      trx.clear();
   }

   {
      account_update_operation aup;
      aup.account = "alice";
      aup.basic = authority( 1, "Hello world!", 1 );
      trx.operations.push_back(aup);
      sign(trx, alice_private_key);
      BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
      trx.clear();

      pop.proposed_ops.emplace_back( aup );
      trx.operations.push_back( pop );
      BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
      pop.proposed_ops.clear();
      trx.clear();
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( simple_single_signature )
{ try {

   ACTORS( (nathan) );
   const asset_object& core = asset_id_type()(db);
   fund("nathan");
   auto old_balance = nathan.balance.amount.value;

   transfer_operation op;
   op.from = "nathan";
   op.to = MUSE_INIT_MINER_NAME;
   op.amount = core.amount(500);
   trx.operations.push_back(op);
   sign(trx, nathan_private_key);
   PUSH_TX( db, trx, database::skip_transaction_dupe_check );

   BOOST_CHECK_EQUAL(get_balance("nathan").amount.value, old_balance - 500);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( any_two_of_three )
{ try {
   ACTORS( (nathan) );
   const asset_object& core = asset_id_type()(db);
   fund("nathan");
   auto old_balance = nathan.balance.amount.value;

   fc::ecc::private_key nathan_key1 = fc::ecc::private_key::regenerate(fc::digest("key1"));
   fc::ecc::private_key nathan_key2 = fc::ecc::private_key::regenerate(fc::digest("key2"));
   fc::ecc::private_key nathan_key3 = fc::ecc::private_key::regenerate(fc::digest("key3"));

   try {
      account_update_operation op;
      op.account = "nathan";
      op.active = authority(2, nathan_key1.get_public_key(), 1, nathan_key2.get_public_key(), 1, nathan_key3.get_public_key(), 1);
      op.owner = *op.active;
      trx.operations.push_back(op);
      sign(trx, nathan_private_key);
      PUSH_TX( db, trx, database::skip_transaction_dupe_check );
      trx.operations.clear();
      trx.signatures.clear();
   } FC_CAPTURE_AND_RETHROW ((nathan.active))

   transfer_operation op;
   op.from = "nathan";
   op.to = MUSE_INIT_MINER_NAME;
   op.amount = core.amount(500);
   trx.operations.push_back(op);
   sign(trx, nathan_key1);
   MUSE_CHECK_THROW(PUSH_TX( db, trx, database::skip_transaction_dupe_check ), fc::exception);
   sign(trx, nathan_key2);
   PUSH_TX( db, trx, database::skip_transaction_dupe_check );
   BOOST_CHECK_EQUAL(get_balance("nathan").amount.value, old_balance - 500);

   trx.signatures.clear();
   sign(trx, nathan_key2);
   sign(trx, nathan_key3);
   PUSH_TX( db, trx, database::skip_transaction_dupe_check );
   BOOST_CHECK_EQUAL(get_balance("nathan").amount.value, old_balance - 1000);

   trx.signatures.clear();
   sign(trx, nathan_key1);
   sign(trx, nathan_key3);
   PUSH_TX( db, trx, database::skip_transaction_dupe_check );
   BOOST_CHECK_EQUAL(get_balance("nathan").amount.value, old_balance - 1500);

   trx.signatures.clear();
   sign(trx,nathan_key3);
   MUSE_CHECK_THROW(PUSH_TX( db, trx, database::skip_transaction_dupe_check ), fc::exception);
   BOOST_CHECK_EQUAL(get_balance("nathan").amount.value, old_balance - 1500);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( recursive_accounts )
{ try {
   ACTORS( (parent1)(parent2)(grandparent) );
   const auto& core = asset_id_type()(db);

   BOOST_TEST_MESSAGE( "Creating child account that requires both parent1 and parent2 to approve" );
   {
      account_create_operation make_child_op;
      make_child_op.creator = MUSE_INIT_MINER_NAME;
      make_child_op.fee = core.amount(1000);
      make_child_op.new_account_name = "child";
      make_child_op.owner = authority(2, "parent1", 1, "parent2", 1);
      make_child_op.active = authority(2, "parent1", 1, "parent2", 1);
      trx.operations.push_back(make_child_op);
      PUSH_TX( db, trx, ~0 );
      trx.operations.clear();
   }
   fund("child");
   auto& child = db.get_account("child");
   auto old_balance = child.balance.amount.value;

   BOOST_TEST_MESSAGE( "Attempting to transfer with no signatures, should fail" );
   transfer_operation op; 
   op.from = "child";
   op.to = MUSE_INIT_MINER_NAME;
   op.amount = core.amount(500);
   trx.operations.push_back(op);
   MUSE_CHECK_THROW(PUSH_TX( db, trx, database::skip_transaction_dupe_check ), fc::exception);

   BOOST_TEST_MESSAGE( "Attempting to transfer with parent1 signature, should fail" );
   sign(trx, parent1_private_key);
   MUSE_CHECK_THROW(PUSH_TX( db, trx, database::skip_transaction_dupe_check ), fc::exception);
   trx.signatures.clear();

   BOOST_TEST_MESSAGE( "Attempting to transfer with parent2 signature, should fail" );
   sign(trx, parent2_private_key);
   MUSE_CHECK_THROW(PUSH_TX( db, trx, database::skip_transaction_dupe_check ), fc::exception);

   BOOST_TEST_MESSAGE( "Attempting to transfer with parent1 and parent2 signature, should succeed" );
   sign(trx, parent1_private_key);
   PUSH_TX( db, trx );
   BOOST_CHECK_EQUAL(get_balance("child").amount.value, old_balance - 500);
   trx.operations.clear();
   trx.signatures.clear();

   BOOST_TEST_MESSAGE( "Adding a key for the child that can override parents" );
   fc::ecc::private_key child_private_key = fc::ecc::private_key::generate();
   {
      account_update_operation op;
      op.account = "child";
      op.active = authority(2, "parent1", 1, "parent2", 1,
                            child_private_key.get_public_key(), 2);
      trx.operations.push_back(op);
      sign(trx, parent1_private_key);
      sign(trx, parent2_private_key);
      PUSH_TX( db, trx );
      BOOST_REQUIRE_EQUAL(child.active.num_auths(), 3);
      trx.operations.clear();
      trx.signatures.clear();
   }

   trx.operations.push_back(op);
   trx.expiration -= fc::seconds(1);  // Survive trx dupe check

   BOOST_TEST_MESSAGE( "Attempting transfer with no signatures, should fail" );
   MUSE_CHECK_THROW(PUSH_TX( db, trx, database::skip_transaction_dupe_check ), fc::exception);
   BOOST_TEST_MESSAGE( "Attempting transfer just parent1, should fail" );
   sign(trx, parent1_private_key);
   MUSE_CHECK_THROW(PUSH_TX( db, trx, database::skip_transaction_dupe_check ), fc::exception);
   trx.signatures.clear();
   BOOST_TEST_MESSAGE( "Attempting transfer just parent2, should fail" );
   sign(trx, parent2_private_key);
   MUSE_CHECK_THROW(PUSH_TX( db, trx, database::skip_transaction_dupe_check ), fc::exception);

   BOOST_TEST_MESSAGE( "Attempting transfer both parents, should succeed" );
   sign(trx, parent1_private_key);
   PUSH_TX( db, trx );
   BOOST_CHECK_EQUAL(get_balance("child").amount.value, old_balance - 1000);
   trx.signatures.clear();

   BOOST_TEST_MESSAGE( "Attempting transfer with just child key, should succeed" );
   trx.expiration -= fc::seconds(1);  // Survive trx dupe check
   sign(trx, child_private_key);
   PUSH_TX( db, trx );
   BOOST_CHECK_EQUAL(get_balance("child").amount.value, old_balance - 1500);
   trx.operations.clear();
   trx.signatures.clear();

   BOOST_TEST_MESSAGE( "Creating grandparent account, parent1 now requires authority of grandparent" );
   {
      generate_blocks( db.head_block_time() + MUSE_OWNER_UPDATE_LIMIT + fc::seconds(5) );
      trx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      account_update_operation op;
      op.account = "parent1";
      op.active = authority(1, "grandparent", 1);
      op.owner = *op.active;
      trx.operations.push_back(op);
      PUSH_TX( db, trx, ~0 );
      trx.operations.clear();
      trx.signatures.clear();
   }

   BOOST_TEST_MESSAGE( "Attempt to transfer using old parent keys, should fail" );
   trx.operations.push_back(op);
   sign(trx, parent1_private_key);
   sign(trx, parent2_private_key);
   MUSE_CHECK_THROW(PUSH_TX( db, trx, database::skip_transaction_dupe_check ), fc::exception);
   trx.signatures.clear();

   BOOST_TEST_MESSAGE( "Attempt to transfer using parent2_key and grandparent_key" );
   sign( trx, parent2_private_key  );
   sign( trx, grandparent_private_key  );
   PUSH_TX( db, trx );
   BOOST_CHECK_EQUAL(get_balance("child").amount.value, old_balance - 2000);
   trx.clear();

   BOOST_TEST_MESSAGE( "Update grandparent account authority to be initminer account" );
   {
      generate_blocks( db.head_block_time() + MUSE_OWNER_UPDATE_LIMIT + fc::seconds(5) );
      trx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      account_update_operation op;
      op.account = "grandparent";
      op.active = authority(1, MUSE_INIT_MINER_NAME, 1);
      op.owner = *op.active;
      trx.operations.push_back(op);
      PUSH_TX( db, trx, ~0 );
      trx.operations.clear();
      trx.signatures.clear();
   }

   BOOST_TEST_MESSAGE( "Create recursion depth failure" );
   trx.operations.push_back(op);
   sign(trx, parent2_private_key);
   sign(trx, grandparent_private_key);
   sign(trx, init_account_priv_key);
   //Fails due to recursion depth.
   MUSE_CHECK_THROW(PUSH_TX( db, trx, database::skip_transaction_dupe_check ), fc::exception);
   BOOST_TEST_MESSAGE( "verify child key can override recursion checks" );
   trx.signatures.clear();
   sign(trx, child_private_key);
   PUSH_TX( db, trx );
   BOOST_CHECK_EQUAL(get_balance("child").amount.value, old_balance - 2500);
   trx.operations.clear();
   trx.signatures.clear();

   BOOST_TEST_MESSAGE( "Verify a cycle fails" );
   {
      generate_blocks( db.head_block_time() + MUSE_OWNER_UPDATE_LIMIT + fc::seconds(5) );
      trx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      account_update_operation op;
      op.account = "parent1";
      op.active = authority(1, "child", 1);
      op.owner = *op.active;
      trx.operations.push_back(op);
      PUSH_TX( db, trx, ~0 );
      trx.operations.clear();
      trx.signatures.clear();
   }

   trx.operations.push_back(op);
   sign(trx, parent2_private_key);
   //Fails due to recursion depth.
   MUSE_CHECK_THROW(PUSH_TX( db, trx ), fc::exception);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposed_single_account )
{ try {
   INVOKE(any_two_of_three);

   ACTORS( (moneyman) );

   fund("moneyman");

   fc::ecc::private_key nathan_key1 = fc::ecc::private_key::regenerate(fc::digest("key1"));
   fc::ecc::private_key nathan_key2 = fc::ecc::private_key::regenerate(fc::digest("key2"));
   fc::ecc::private_key nathan_key3 = fc::ecc::private_key::regenerate(fc::digest("key3"));
   const asset_object& core = asset_id_type()(db);

   //Following any_two_of_three, nathan's active authority is satisfied by any two of {key1,key2,key3}
   BOOST_TEST_MESSAGE( "moneyman is creating proposal for nathan to transfer 100 CORE to moneyman" );

   transfer_operation transfer_op;
   transfer_op.from = "nathan";
   transfer_op.to  = "moneyman";
   transfer_op.amount = core.amount(100); 

   proposal_create_operation op;
   op.proposed_ops.emplace_back( transfer_op );
   op.expiration_time =  db.head_block_time() + fc::days(1);

   auto nathan_start_balance = get_balance("nathan").amount.value;
   {
      vector<authority> other;
      flat_set<string> active_set, owner_set, basic_set, master_set, comp_set;
      operation_get_required_authorities(op,active_set,owner_set,basic_set,master_set,comp_set,other);
      BOOST_CHECK_EQUAL(active_set.size(), 0);
      BOOST_CHECK_EQUAL(owner_set.size(), 0);
      BOOST_CHECK_EQUAL(basic_set.size(), 0);
      BOOST_CHECK_EQUAL(master_set.size(), 0);
      BOOST_CHECK_EQUAL(comp_set.size(), 0);
      BOOST_CHECK_EQUAL(other.size(), 0);

      operation_get_required_authorities(op.proposed_ops.front().op,active_set,owner_set,basic_set,master_set,comp_set,other);
      BOOST_CHECK_EQUAL(active_set.size(), 1);
      BOOST_CHECK_EQUAL(owner_set.size(), 0);
      BOOST_CHECK_EQUAL(basic_set.size(), 0);
      BOOST_CHECK_EQUAL(master_set.size(), 0);
      BOOST_CHECK_EQUAL(comp_set.size(), 0);
      BOOST_CHECK_EQUAL(other.size(), 0);
      BOOST_CHECK_EQUAL("nathan", *active_set.begin());
   }

   trx.operations.push_back(op);

   PUSH_TX( db, trx );
   const proposal_object& proposal = *db.get_index_type<proposal_index>().indices().begin();

   BOOST_CHECK_EQUAL(proposal.required_active_approvals.size(), 1);
   BOOST_CHECK_EQUAL(proposal.available_active_approvals.size(), 0);
   BOOST_CHECK_EQUAL(proposal.required_owner_approvals.size(), 0);
   BOOST_CHECK_EQUAL(proposal.available_owner_approvals.size(), 0);
   BOOST_CHECK_EQUAL("nathan", *proposal.required_active_approvals.begin());

   proposal_update_operation pup;
   pup.proposal = proposal.id;
   BOOST_TEST_MESSAGE( "Updating the proposal to have nathan's authority" );
   pup.active_approvals_to_add.insert("nathan");

   trx.operations = {pup};
   sign( trx, init_account_priv_key );
   // init_miner may not add nathan's approval.
   MUSE_CHECK_THROW(PUSH_TX( db, trx ), fc::exception);

   pup.active_approvals_to_add.clear();
   pup.active_approvals_to_add.insert( MUSE_INIT_MINER_NAME );
   trx.operations = {pup};
   sign( trx, init_account_priv_key );
   // init_miner has no stake in the transaction.
   MUSE_CHECK_THROW(PUSH_TX( db, trx ), fc::exception);

   trx.signatures.clear();
   pup.active_approvals_to_add.clear();
   pup.active_approvals_to_add.insert("nathan");
      
   trx.operations = {pup};
   sign( trx, nathan_key3 );
   sign( trx, nathan_key2 );

   BOOST_CHECK_EQUAL(get_balance("nathan").amount.value, nathan_start_balance);
   PUSH_TX( db, trx );
   BOOST_CHECK_EQUAL(get_balance("nathan").amount.value, nathan_start_balance - 100);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposal_two_accounts )
{ try {
   generate_block();

   ACTORS( (nathan)(dan) );
   fund("nathan");
   fund("dan");

   {
      transfer_operation top;
      top.from = "dan";
      top.to = "nathan";
      top.amount = asset(500);

      proposal_create_operation pop;
      pop.proposed_ops.emplace_back(top);
      std::swap(top.from, top.to);
      pop.proposed_ops.emplace_back(top);

      pop.expiration_time = db.head_block_time() + fc::days(1);
      trx.operations.push_back(pop);
      PUSH_TX( db, trx );
      trx.clear();
   }

   const proposal_object& prop = *db.get_index_type<proposal_index>().indices().begin();
   BOOST_CHECK_EQUAL(2, prop.required_active_approvals.size());
   BOOST_CHECK_EQUAL(0, prop.required_owner_approvals.size());
   BOOST_CHECK(!prop.is_authorized_to_execute(db));

   {
      proposal_id_type pid = prop.id;
      proposal_update_operation uop;
      uop.proposal = prop.id;
      uop.active_approvals_to_add.insert("nathan");
      trx.operations.push_back(uop);
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
      trx.clear();

      BOOST_CHECK(db.find_object(pid) != nullptr);
      BOOST_CHECK(!prop.is_authorized_to_execute(db));

      uop.active_approvals_to_add = {"dan"};
      trx.operations.push_back(uop);
      sign( trx, nathan_private_key );
      MUSE_REQUIRE_THROW(PUSH_TX( db, trx ), fc::exception);
      trx.signatures.clear();
      sign( trx, dan_private_key );
      PUSH_TX( db, trx );

      BOOST_CHECK(db.find_object(pid) == nullptr);
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposal_delete )
{ try {
   generate_block();

   ACTORS( (nathan)(dan) );
   fund("nathan");
   fund("dan");
   const auto& nathan_orig_balance = nathan.balance.amount.value;

   {
      transfer_operation top;
      top.from = "dan";
      top.to = "nathan";
      top.amount = asset(500);

      proposal_create_operation pop;
      pop.proposed_ops.emplace_back(top);
      std::swap(top.from, top.to);
      top.amount = asset(6000);
      pop.proposed_ops.emplace_back(top);

      pop.expiration_time = db.head_block_time() + fc::days(1);
      trx.operations.push_back(pop);
      PUSH_TX( db, trx );
      trx.clear();
   }

   const proposal_object& prop = *db.get_index_type<proposal_index>().indices().begin();
   BOOST_CHECK_EQUAL(2, prop.required_active_approvals.size());
   BOOST_CHECK_EQUAL(0, prop.required_owner_approvals.size());
   BOOST_CHECK(!prop.is_authorized_to_execute(db));

   {
      proposal_update_operation uop;
      uop.proposal = prop.id;
      uop.active_approvals_to_add.insert("nathan");
      trx.operations.push_back(uop);
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
      trx.clear();
      BOOST_CHECK(!prop.is_authorized_to_execute(db));
      BOOST_CHECK_EQUAL(prop.available_active_approvals.size(), 1);

      std::swap(uop.active_approvals_to_add, uop.active_approvals_to_remove);
      trx.operations.push_back(uop);
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
      trx.clear();
      BOOST_CHECK(!prop.is_authorized_to_execute(db));
      BOOST_CHECK_EQUAL(prop.available_active_approvals.size(), 0);
   }

   {
      proposal_id_type pid = prop.id;
      proposal_delete_operation dop;
      dop.proposal = pid;
      dop.vetoer = "nathan";
      trx.operations.push_back(dop);
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
      BOOST_CHECK(db.find_object(pid) == nullptr);
      BOOST_CHECK_EQUAL(get_balance("nathan").amount.value, nathan_orig_balance);
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposal_owner_authority_delete )
{ try {
   generate_block();

   ACTORS( (nathan)(dan) );
   fund("nathan");
   fund("dan");
   const auto& nathan_orig_balance = nathan.balance.amount.value;

   {
      transfer_operation top;
      top.from = "dan";
      top.to = "nathan";
      top.amount = asset(500);

      account_update_operation uop;
      uop.account = "nathan";
      uop.owner = authority(1, generate_private_key("nathan2").get_public_key(), 1);

      proposal_create_operation pop;
      pop.proposed_ops.emplace_back(top);
      pop.proposed_ops.emplace_back(uop);
      std::swap(top.from, top.to);
      top.amount = asset(6000);
      pop.proposed_ops.emplace_back(top);

      pop.expiration_time = db.head_block_time() + fc::days(1);
      trx.operations.push_back(pop);
      PUSH_TX( db, trx );
      trx.clear();
   }

   const proposal_object& prop = *db.get_index_type<proposal_index>().indices().begin();
   BOOST_CHECK_EQUAL(prop.required_active_approvals.size(), 1);
   BOOST_CHECK_EQUAL(prop.required_owner_approvals.size(), 1);
   BOOST_CHECK(!prop.is_authorized_to_execute(db));

   {
      proposal_update_operation uop;
      uop.proposal = prop.id;
      uop.owner_approvals_to_add.insert("nathan");
      trx.operations.push_back(uop);
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
      trx.clear();
      BOOST_CHECK(!prop.is_authorized_to_execute(db));
      BOOST_CHECK_EQUAL(prop.available_owner_approvals.size(), 1);

      std::swap(uop.owner_approvals_to_add, uop.owner_approvals_to_remove);
      trx.operations.push_back(uop);
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
      trx.clear();
      BOOST_CHECK(!prop.is_authorized_to_execute(db));
      BOOST_CHECK_EQUAL(prop.available_owner_approvals.size(), 0);
   }

   {
      proposal_id_type pid = prop.id;
      proposal_delete_operation dop;
      dop.proposal = pid;
      dop.type = proposal_delete_operation::authority_type::owner;
      dop.vetoer = "nathan";
      trx.operations.push_back(dop);
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
      BOOST_CHECK(db.find_object(pid) == nullptr);
      BOOST_CHECK_EQUAL(get_balance("nathan").amount.value, nathan_orig_balance);
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposal_owner_authority_complete )
{ try {
   generate_block();

   ACTORS( (nathan)(dan) );
   fund("nathan");
   fund("dan");

   {
      transfer_operation top;
      top.from = "dan";
      top.to = "nathan";
      top.amount = asset(500);

      account_update_operation uop;
      uop.account = "nathan";
      uop.owner = authority(1, generate_private_key("nathan2").get_public_key(), 1);

      proposal_create_operation pop;
      pop.proposed_ops.emplace_back(top);
      pop.proposed_ops.emplace_back(uop);
      std::swap(top.from, top.to);
      top.amount = asset(6000);
      pop.proposed_ops.emplace_back(top);

      pop.expiration_time = db.head_block_time() + fc::days(1);
      trx.operations.push_back(pop);
      PUSH_TX( db, trx );
      trx.clear();
   }

   const proposal_object& prop = *db.get_index_type<proposal_index>().indices().begin();
   BOOST_CHECK_EQUAL(prop.required_active_approvals.size(), 1);
   BOOST_CHECK_EQUAL(prop.required_owner_approvals.size(), 1);
   BOOST_CHECK(!prop.is_authorized_to_execute(db));

   {
      proposal_id_type pid = prop.id;
      proposal_update_operation uop;
      uop.proposal = prop.id;
      uop.key_approvals_to_add.insert(dan.active.key_auths.begin()->first);
      trx.operations.push_back(uop);
      //sign( trx, nathan_private_key );
      sign( trx, dan_private_key );
      PUSH_TX( db, trx );
      trx.clear();
      BOOST_CHECK(!prop.is_authorized_to_execute(db));
      BOOST_CHECK_EQUAL(prop.available_key_approvals.size(), 1);

      std::swap(uop.key_approvals_to_add, uop.key_approvals_to_remove);
      trx.operations.push_back(uop);
      trx.expiration -= fc::seconds(1);  // Survive trx dupe check
      //sign( trx, nathan_private_key );
      sign( trx, dan_private_key );
      PUSH_TX( db, trx );
      trx.clear();
      BOOST_CHECK(!prop.is_authorized_to_execute(db));
      BOOST_CHECK_EQUAL(prop.available_key_approvals.size(), 0);

      std::swap(uop.key_approvals_to_add, uop.key_approvals_to_remove);
      trx.operations.push_back(uop);
      trx.expiration -= fc::seconds(1);  // Survive trx dupe check
      //sign( trx, nathan_private_key );
      sign( trx, dan_private_key );
      PUSH_TX( db, trx );
      trx.clear();
      BOOST_CHECK(!prop.is_authorized_to_execute(db));
      BOOST_CHECK_EQUAL(prop.available_key_approvals.size(), 1);

      uop.key_approvals_to_add.clear();
      uop.owner_approvals_to_add.insert("nathan");
      trx.operations.push_back(uop);
      trx.expiration -= fc::seconds(1);  // Survive trx dupe check
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
      trx.clear();
      BOOST_CHECK(db.find_object(pid) == nullptr);
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( bogus_signature )
{ try {
   ACTORS( (alice)(bob)(charlie) );
   fund("alice");
   const asset_object& core = asset_id_type()(db);

   transfer_operation xfer_op;
   xfer_op.from = "alice";
   xfer_op.to = "bob";
   xfer_op.amount = core.amount(5000);

   trx.clear();
   trx.operations.push_back( xfer_op );

   BOOST_TEST_MESSAGE( "Transfer signed by alice" );
   sign( trx, alice_private_key );

   flat_set<string> active_set, owner_set, basic_set, master_set, comp_set;
   vector<authority> others;
   trx.get_required_authorities( active_set, owner_set, basic_set, master_set, comp_set, others );

   PUSH_TX( db, trx );

   trx.operations.push_back( xfer_op );
   BOOST_TEST_MESSAGE( "Invalidating Alices Signature" );
   // Alice's signature is now invalid
   MUSE_REQUIRE_THROW( PUSH_TX( db, trx ), fc::exception );
   // Re-sign, now OK (sig is replaced)
   BOOST_TEST_MESSAGE( "Resign with Alice's Signature" );
   trx.signatures.clear();
   sign( trx, alice_private_key );
   PUSH_TX( db, trx );

   trx.signatures.clear();
   trx.operations.push_back( xfer_op );
   sign( trx, alice_private_key );
   sign( trx, charlie_private_key );
   // Signed by third-party Charlie (irrelevant key, not in authority)
   MUSE_REQUIRE_THROW( PUSH_TX( db, trx ), tx_irrelevant_sig );
} FC_LOG_AND_RETHROW() }

/*
 * Simple corporate accounts:
 *
 * Well Corp.       Alice 50, Bob 50             T=60
 * Xylo Company     Alice 30, Cindy 50           T=40
 * Yaya Inc.        Bob 10, Dan 10, Edy 10       T=20
 * Zyzz Co.         Dan 50                       T=40
 *
 * Complex corporate accounts:
 *
 * Mega Corp.       Well 30, Yes 30              T=40
 * Nova Ltd.        Alice 10, Well 10            T=20
 * Odle Intl.       Dan 10, Yes 10, Zyzz 10      T=20
 * Poxx LLC         Well 10, Xylo 10, Yes 20, Zyzz 20   T=40
 */

BOOST_AUTO_TEST_CASE( get_required_signatures_test )
{ try {
   ACTORS( (alice)(bob)(cindy)(dan)(edy)
           (mega)(nova)(odle)(poxx)
           (well)(xylo)(yaya)(zyzz)
         );
   fund("alice");
   fund("bob");
   fund("cindy");
   fund("dan");
   fund("edy");
   fund("mega");
   fund("nova");
   fund("odle");
   fund("poxx");
   fund("well");
   fund("xylo");
   fund("yaya");
   fund("zyzz");

   auto set_auth = [this]( const string& account, const authority& auth )
   {
      signed_transaction tx;
      account_update_operation op;
      op.account = account;
      op.active = auth;
      op.owner = auth;
      tx.operations.push_back( op );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      PUSH_TX( db, tx, database::skip_transaction_signatures | database::skip_authority_check );
   };

   auto get_active = [this]( const string& account )
      -> const authority* { return &(db.get_account(account).active); };

   auto get_owner = [this]( const string& account )
      -> const authority* { return &(db.get_account(account).owner); };

   auto get_basic = [this]( const string& account )
      -> const authority* { return &(db.get_account(account).basic); };

   auto get_master = [this]( const string& url )
      -> const authority* { return &(db.get_content(url).manage_master); };

   auto get_comp = [this]( const string& url )
      -> const authority* { return &(db.get_content(url).manage_comp); };

   auto chk = [this,get_active,get_owner,get_basic,get_master,get_comp]
              ( const signed_transaction& tx, flat_set<public_key_type> available_keys,
                set<public_key_type> ref_set )
      -> bool {
         set<public_key_type> result_set = tx.get_required_signatures( db.get_chain_id(), available_keys,
                                                                       get_active, get_owner, get_basic,
                                                                       get_master, get_comp );
         return result_set == ref_set;
      };

   set_auth( "well", authority( 60, "alice", 50, "bob", 50 ) );
   set_auth( "xylo", authority( 40, "alice", 30, "cindy", 50 ) );
   set_auth( "yaya", authority( 20, "bob", 10, "dan", 10, "edy", 10 ) );
   set_auth( "zyzz", authority( 40, "dan", 50 ) );

   set_auth( "mega", authority( 40, "well", 30, "yaya", 30 ) );
   set_auth( "nova", authority( 20, "alice", 10, "well", 10 ) );
   set_auth( "odle", authority( 20, "dan", 10, "yaya", 10, "zyzz", 10 ) );
   set_auth( "poxx", authority( 40, "well", 10, "xylo", 10, "yaya", 20, "zyzz", 20 ) );

   flat_set< public_key_type > all_keys
         { alice_public_key, bob_public_key, cindy_public_key, dan_public_key, edy_public_key };

   trx.operations.push_back( transfer_operation() );
   transfer_operation& op = trx.operations.back().get<transfer_operation>();
   op.to = "edy";
   op.amount = asset(1);

   op.from = "alice";
   BOOST_CHECK( chk( trx, all_keys, { alice_public_key } ) );
   op.from = "bob";
   BOOST_CHECK( chk( trx, all_keys, { bob_public_key } ) );
   op.from = "well";
   BOOST_CHECK( chk( trx, all_keys, { alice_public_key, bob_public_key } ) );
   op.from = "xylo";
   BOOST_CHECK( chk( trx, all_keys, { alice_public_key, cindy_public_key } ) );
   op.from = "yaya";
   BOOST_CHECK( chk( trx, all_keys, { bob_public_key, dan_public_key } ) );
   op.from = "zyzz";
   BOOST_CHECK( chk( trx, all_keys, { dan_public_key } ) );

   op.from = "mega";
   BOOST_CHECK( chk( trx, all_keys, { alice_public_key, bob_public_key, dan_public_key } ) );
   op.from = "nova";
   BOOST_CHECK( chk( trx, all_keys, { alice_public_key, bob_public_key } ) );
   op.from = "odle";
   BOOST_CHECK( chk( trx, all_keys, { bob_public_key, dan_public_key } ) );
   op.from = "poxx";
   BOOST_CHECK( chk( trx, all_keys, { alice_public_key, bob_public_key, cindy_public_key, dan_public_key } ) );

   // TODO:  Add sigs to tx, then check
   // TODO:  Check removing sigs      
   // TODO:  Accounts with mix of keys and accounts in their authority
   // TODO:  Tx with multiple ops requiring different sigs
} FC_LOG_AND_RETHROW() }

/*
 * Pathological case
 *
 *      Roco(T=2)
 *    1/         \2
 *   Styx(T=2)   Thud(T=1)
 *  1/     \1       |1
 * Alice  Bob     Alice
 */
BOOST_AUTO_TEST_CASE( nonminimal_sig_test )
{ try {
   ACTORS( (alice)(bob)(roco)(styx)(thud) );
   fund("roco");

   auto set_auth = [this]( const string& account, const authority& auth )
   {
      signed_transaction tx;
      account_update_operation op;
      op.account = account;
      op.active = auth;
      op.owner = auth;
      tx.operations.push_back( op );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      PUSH_TX( db, tx, database::skip_transaction_signatures | database::skip_authority_check );
   };

   auto get_active = [this]( const string& account )
      -> const authority* { return &(db.get_account(account).active); };

   auto get_owner = [this]( const string& account )
      -> const authority* { return &(db.get_account(account).owner); };

   auto get_basic = [this]( const string& account )
      -> const authority* { return &(db.get_account(account).basic); };

   auto get_master = [this]( const string& url )
      -> const authority* { return &(db.get_content(url).manage_master); };

   auto get_comp = [this]( const string& url )
      -> const authority* { return &(db.get_content(url).manage_comp); };

   auto chk = [this,get_active,get_owner,get_basic,get_master,get_comp]
              ( const signed_transaction& tx, flat_set<public_key_type> available_keys,
                set<public_key_type> ref_set )
      -> bool {
         set<public_key_type> result_set = tx.get_required_signatures( db.get_chain_id(), available_keys,
                                                                       get_active, get_owner, get_basic,
                                                                       get_master, get_comp );
         return result_set == ref_set;
      };

   auto chk_min = [this,get_active,get_owner,get_basic,get_master,get_comp]
                  ( const signed_transaction& tx, flat_set<public_key_type> available_keys,
                    set<public_key_type> ref_set )
      -> bool
      {
         set<public_key_type> result_set = tx.minimize_required_signatures( db.get_chain_id(), available_keys,
                                                                            get_active, get_owner, get_basic,
                                                                            get_master, get_comp, 2 );
         return result_set == ref_set;
      };

   set_auth( "roco", authority( 2, "styx", 1, "thud", 2 ) );
   set_auth( "styx", authority( 2, "alice", 1, "bob", 1 ) );
   set_auth( "thud", authority( 1, "alice", 1 ) );

   transfer_operation op;
   op.from = "roco";
   op.to = "bob";
   op.amount = asset(1);
   trx.operations.push_back( op );

   BOOST_CHECK( chk( trx, { alice_public_key, bob_public_key }, { alice_public_key, bob_public_key } ) );
   BOOST_CHECK( chk_min( trx, { alice_public_key, bob_public_key }, { alice_public_key } ) );

   MUSE_REQUIRE_THROW( trx.verify_authority( db.get_chain_id(), get_active, get_owner, get_basic, get_master, get_comp, 1 ), fc::exception );
   MUSE_REQUIRE_THROW( trx.verify_authority( db.get_chain_id(), get_active, get_owner, get_basic, get_master, get_comp, 2 ), fc::exception );
   sign( trx, alice_private_key );
   trx.verify_authority( db.get_chain_id(), get_active, get_owner, get_basic, get_master, get_comp, 1 );
   trx.verify_authority( db.get_chain_id(), get_active, get_owner, get_basic, get_master, get_comp, 2 );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( parent_owner_test )
{ try {
   ACTORS( (alice)(bob) );
   fund("bob");

   auto set_auth2 = [this]( const string& account, const authority& active,
                            const authority& owner )
   {
      signed_transaction tx;
      account_update_operation op;
      op.account = account;
      op.active = active;
      op.owner = owner;
      tx.operations.push_back( op );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      PUSH_TX( db, tx, database::skip_transaction_signatures | database::skip_authority_check );
   } ;

   auto set_auth = [&set_auth2]( const string& account, const authority& auth )
      { set_auth2( account, auth, auth ); };

   auto get_active = [this]( const string& account )
      -> const authority* { return &(db.get_account(account).active); };

   auto get_owner = [this]( const string& account )
      -> const authority* { return &(db.get_account(account).owner); };

   auto get_basic = [this]( const string& account )
      -> const authority* { return &(db.get_account(account).basic); };

   auto get_master = [this]( const string& url )
      -> const authority* { return &(db.get_content(url).manage_master); };

   auto get_comp = [this]( const string& url )
      -> const authority* { return &(db.get_content(url).manage_comp); };

   auto chk = [this,get_active,get_owner,get_basic,get_master,get_comp]
              ( const signed_transaction& tx, flat_set<public_key_type> available_keys,
                set<public_key_type> ref_set )
      -> bool {
         set<public_key_type> result_set = tx.get_required_signatures( db.get_chain_id(), available_keys,
                                                                       get_active, get_owner, get_basic,
                                                                       get_master, get_comp );
         return result_set == ref_set;
      };

   fc::ecc::private_key alice_active_key = fc::ecc::private_key::regenerate(fc::digest("alice_active"));
   fc::ecc::private_key alice_owner_key = fc::ecc::private_key::regenerate(fc::digest("alice_owner"));
   public_key_type alice_active_pub( alice_active_key.get_public_key() );
   public_key_type alice_owner_pub( alice_owner_key.get_public_key() );
   set_auth2( "alice", authority( 1, alice_active_pub, 1 ), authority( 1, alice_owner_pub, 1 ) );
   set_auth( "bob", authority( 1, "alice", 1 ) );

   transfer_operation op;
   op.from = "bob";
   op.to = "alice";
   op.amount = asset(1);
   trx.operations.push_back( op );

   BOOST_CHECK( chk( trx, { alice_owner_pub }, { } ) );
   BOOST_CHECK( chk( trx, { alice_active_pub, alice_owner_pub }, { alice_active_pub } ) );
   sign( trx, alice_owner_key );
   MUSE_REQUIRE_THROW( trx.verify_authority( db.get_chain_id(), get_active, get_owner, get_basic, get_master, get_comp, 1 ), fc::exception );
   MUSE_REQUIRE_THROW( trx.verify_authority( db.get_chain_id(), get_active, get_owner, get_basic, get_master, get_comp, 2 ), fc::exception );

   trx.signatures.clear();
   sign( trx, alice_active_key );
   trx.verify_authority( db.get_chain_id(), get_active, get_owner, get_basic, get_master, get_comp, 1 );
   trx.verify_authority( db.get_chain_id(), get_active, get_owner, get_basic, get_master, get_comp, 2 );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposal_expires )
{ try {
   generate_block();

   ACTORS( (alice)(bob) );
   fund("alice");
   const asset_object& core = asset_id_type()(db);
   const auto original_balance = alice.balance.amount.value;

   BOOST_TEST_MESSAGE( "Alice creates a proposal and lets it expire" );

   transfer_operation transfer_op;
   transfer_op.from = "alice";
   transfer_op.to  = "bob";
   transfer_op.amount = core.amount(100);

   proposal_create_operation op;
   op.proposed_ops.emplace_back( transfer_op );
   op.expiration_time = db.head_block_time() + fc::minutes(1);
   trx.operations.push_back(op);
   PUSH_TX( db, trx );

   const auto& pidx = db.get_index_type<proposal_index>().indices().get<by_id>();
   BOOST_CHECK_EQUAL( 1, pidx.size() );
   const proposal_object& proposal = *pidx.begin();
   const proposal_id_type pid = proposal.id;

   BOOST_CHECK_EQUAL(proposal.required_active_approvals.size(), 1);
   BOOST_CHECK_EQUAL(proposal.available_active_approvals.size(), 0);
   BOOST_CHECK_EQUAL(proposal.required_owner_approvals.size(), 0);
   BOOST_CHECK_EQUAL(proposal.available_owner_approvals.size(), 0);
   BOOST_CHECK_EQUAL("alice", *proposal.required_active_approvals.begin());

   generate_blocks( op.expiration_time );
   generate_block();

   // Proposal didn't execute
   BOOST_CHECK_EQUAL(get_balance("alice").amount.value, original_balance);
   // Proposal was deleted
   BOOST_CHECK_EQUAL( 0, pidx.size() );
   BOOST_CHECK_THROW( db.get( pid ), fc::assert_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposal_executes_at_expiry )
{ try {
   generate_block();

   ACTORS( (alice)(bob) );
   const asset_object& core = asset_id_type()(db);

   BOOST_TEST_MESSAGE( "Alice creates a proposal and lets it expire" );

   transfer_operation transfer_op;
   transfer_op.from = "alice";
   transfer_op.to  = "bob";
   transfer_op.amount = core.amount(100);

   proposal_create_operation op;
   op.proposed_ops.emplace_back( transfer_op );
   op.expiration_time = db.head_block_time() + fc::minutes(1);
   trx.operations.push_back(op);
   PUSH_TX( db, trx );
   trx.clear();

   const auto& pidx = db.get_index_type<proposal_index>().indices().get<by_id>();
   BOOST_CHECK_EQUAL( 1, pidx.size() );
   const proposal_id_type pid = pidx.begin()->id;

   proposal_update_operation uop;
   uop.proposal = pid;
   uop.active_approvals_to_add.insert("alice");
   trx.operations.push_back(uop);
   sign( trx, alice_private_key );
   PUSH_TX( db, trx );
   trx.clear();

   // Proposal failed to execute
   BOOST_CHECK_EQUAL( 1, pidx.size() );

   const proposal_object& proposal = pid(db);
   BOOST_CHECK_EQUAL(proposal.required_active_approvals.size(), 1);
   BOOST_CHECK_EQUAL(proposal.available_active_approvals.size(), 1);
   BOOST_CHECK_EQUAL(proposal.required_owner_approvals.size(), 0);
   BOOST_CHECK_EQUAL(proposal.available_owner_approvals.size(), 0);
   BOOST_CHECK_EQUAL("alice", *proposal.required_active_approvals.begin());

   fund( "alice", 1000 );

   // Proposal didn't execute yet
   BOOST_CHECK_EQUAL( 1, pidx.size() );
   BOOST_CHECK_EQUAL(get_balance("alice").amount.value, 1000);

   generate_blocks( op.expiration_time );
   generate_block();

   // Proposal did execute now
   BOOST_CHECK_EQUAL(get_balance("alice").amount.value, 1000 - transfer_op.amount.amount.value);

   // Proposal was deleted
   BOOST_CHECK_EQUAL( 0, pidx.size() );
   BOOST_CHECK_THROW( db.get( pid ), fc::assert_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( proposals_with_mixed_authorities )
{ try {
   generate_block();

   ACTORS( (alice)(bob)(marge)(miranda)(monica)(muriel)(paula)(penny)(uhura)(vanessa)(veronica) );
   const asset_object& core = asset_id_type()(db);
   const auto& pidx = db.get_index_type<proposal_index>().indices().get<by_id>();
   fund( "alice" );

   {
      transfer_operation transfer_op; // requires active auth
      transfer_op.from = "alice";
      transfer_op.to  = "bob";
      transfer_op.amount = core.amount(100);

      friendship_operation friend_op; // requires basic auth
      friend_op.who = "alice";
      friend_op.whom = "bob";
      proposal_create_operation op;
      op.proposed_ops.emplace_back( transfer_op );
      op.proposed_ops.emplace_back( friend_op );
      op.expiration_time = db.head_block_time() + fc::minutes(1);
      trx.operations.push_back( op );
      PUSH_TX( db, trx ); // works, because active overrides basic
      trx.clear();

      op.proposed_ops.clear();
      std::swap( friend_op.who, friend_op.whom );
      op.proposed_ops.emplace_back( transfer_op );
      op.proposed_ops.emplace_back( friend_op );
      trx.operations.push_back( op );
      // fails: combination of alice's active + bob's basic auth is not allowed
      BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
      trx.clear();

      BOOST_CHECK_EQUAL( 1, pidx.size() );
      const proposal_id_type pid = pidx.begin()->id;
      const proposal_object& proposal = pid(db);
      BOOST_CHECK_EQUAL( 1, proposal.required_active_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal.available_active_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal.required_owner_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal.available_owner_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal.required_basic_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal.available_basic_approvals.size() );
      BOOST_CHECK_EQUAL( "alice", *proposal.required_active_approvals.begin() );

      proposal_update_operation pup;
      pup.proposal = pid;
      pup.active_approvals_to_add.insert( "alice" );
      trx.operations.push_back( pup );
      sign( trx, alice_private_key );
      PUSH_TX( db, trx );
      trx.clear();

      BOOST_CHECK( pidx.empty() );
   }

   {
      content_operation cop;
      cop.uploader = "uhura";
      cop.url = "ipfs://abcdefg1";
      cop.album_meta.album_title = "Albuquerque";
      cop.track_meta.track_title = "Tracksonville";
      cop.comp_meta.third_party_publishers = false;
      distribution dist;
      dist.payee = "paula";
      dist.bp = 10000;
      cop.distributions.push_back( dist );
      management_vote mgmt;
      mgmt.voter = "miranda";
      mgmt.percentage = 50;
      cop.management.push_back( mgmt );
      mgmt.voter = "muriel";
      cop.management.push_back( mgmt );
      cop.management_threshold = 40;
      trx.operations.push_back( cop );

      cop.url = "ipfs://abcdefg2";
      cop.track_meta.track_title = "Track Stop";
      cop.comp_meta.third_party_publishers = true;
      dist.payee = "penny";
      mgmt.voter = "marge";
      cop.management_comp = std::move( vector<management_vote>() );
      cop.management_comp->push_back( mgmt );
      mgmt.voter = "monica";
      cop.management_comp->push_back( mgmt );
      cop.management_threshold_comp = 60;
      dist.payee = "penny";
      cop.distributions_comp = std::move( vector<distribution>() );
      cop.distributions_comp->push_back( dist );
      trx.operations.push_back( cop );
      sign( trx, uhura_private_key );
      PUSH_TX( db, trx );
      trx.clear();

      const auto& content1 = db.get_content( "ipfs://abcdefg1" );
      BOOST_CHECK_EQUAL( 1000, content1.playing_reward );

      const auto& content2 = db.get_content( "ipfs://abcdefg2" );
      BOOST_CHECK_EQUAL( 60, content2.manage_comp.weight_threshold );
   }

   {
      proposal_create_operation pc;

      transfer_operation transfer_op; // requires active auth
      transfer_op.from = "alice";
      transfer_op.to  = "bob";
      transfer_op.amount = core.amount(100);
      pc.proposed_ops.emplace_back( transfer_op );

      friendship_operation friend_op; // requires basic auth
      friend_op.who = "paula";
      friend_op.whom = "bob";
      pc.proposed_ops.emplace_back( friend_op );

      content_update_operation cup;
      cup.url = "ipfs://abcdefg1";
      cup.side = content_update_operation::side_t::master;
      cup.new_playing_reward = 500;
      pc.proposed_ops.emplace_back( cup );
      pc.expiration_time = db.head_block_time() + fc::minutes(1);
      trx.operations.push_back( pc );
      BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
      trx.clear();

      pc.proposed_ops.clear();
      pc.proposed_ops.emplace_back( transfer_op );
      pc.proposed_ops.emplace_back( cup );
      trx.operations.push_back( pc );
      PUSH_TX( db, trx );
      trx.clear();

      BOOST_CHECK_EQUAL( 1, pidx.size() );
      proposal_id_type pid = pidx.begin()->id;
      const proposal_object& proposal1 = pid(db);
      BOOST_CHECK_EQUAL( 3, proposal1.required_active_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal1.available_active_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal1.required_owner_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal1.available_owner_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal1.required_basic_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal1.available_basic_approvals.size() );
      BOOST_CHECK( proposal1.required_active_approvals.find( "alice" ) != proposal1.required_active_approvals.end() );
      BOOST_CHECK( proposal1.required_active_approvals.find( "miranda" ) != proposal1.required_active_approvals.end() );
      BOOST_CHECK( proposal1.required_active_approvals.find( "muriel" ) != proposal1.required_active_approvals.end() );

      proposal_update_operation pup;
      pup.proposal = pid;
      pup.active_approvals_to_add.insert( "alice" );
      trx.operations.push_back( pup );
      sign( trx, alice_private_key );
      PUSH_TX( db, trx );
      trx.clear();

      BOOST_CHECK_EQUAL( 1, pidx.size() );
      BOOST_CHECK_EQUAL( proposal1.available_active_approvals.size(), 1 );

      pup.active_approvals_to_add.clear();
      pup.active_approvals_to_add.insert( "muriel" );
      trx.operations.push_back( pup );
      sign( trx, muriel_private_key );
      PUSH_TX( db, trx );
      trx.clear();

      // Proposal was executed
      BOOST_CHECK_EQUAL( 0, pidx.size() );
      const auto& content1 = db.get_content( "ipfs://abcdefg1" );
      BOOST_CHECK_EQUAL( 500, content1.playing_reward );


      pc.proposed_ops.clear();
      cup.url = "ipfs://abcdefg2";
      cup.side = content_update_operation::side_t::publisher;
      management_vote mgmt;
      mgmt.voter = "marge";
      mgmt.percentage = 50;
      cup.new_management.push_back( mgmt );
      mgmt.voter = "muriel";
      cup.new_management.push_back( mgmt );
      cup.new_threshold = 40;
      cup.new_playing_reward = 0;
      cup.new_publishers_share = 0;
      pc.proposed_ops.emplace_back( cup );
      trx.operations.push_back( pc );
      PUSH_TX( db, trx );
      trx.clear();

      BOOST_CHECK_EQUAL( 1, pidx.size() );
      pid = pidx.begin()->id;
      const proposal_object& proposal2 = pid(db);
      BOOST_CHECK_EQUAL( 2, proposal2.required_active_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal2.available_active_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal2.required_owner_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal2.available_owner_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal2.required_basic_approvals.size() );
      BOOST_CHECK_EQUAL( 0, proposal2.available_basic_approvals.size() );
      BOOST_CHECK( proposal2.required_active_approvals.find( "marge" ) != proposal2.required_active_approvals.end() );
      BOOST_CHECK( proposal2.required_active_approvals.find( "monica" ) != proposal2.required_active_approvals.end() );

      pup.proposal = pid;
      trx.operations.push_back( pup );
      sign( trx, muriel_private_key );
      BOOST_CHECK_THROW( PUSH_TX( db, trx ), fc::assert_exception );
      trx.clear();

      pup.active_approvals_to_add.clear();
      pup.active_approvals_to_add.insert( "marge" );
      trx.operations.push_back( pup );
      sign( trx, marge_private_key );
      PUSH_TX( db, trx );
      trx.clear();

      BOOST_CHECK_EQUAL( 1, pidx.size() );
      BOOST_CHECK_EQUAL( 1, proposal2.available_active_approvals.size() );

      pup.active_approvals_to_add.clear();
      pup.active_approvals_to_add.insert( "monica" );
      trx.operations.push_back( pup );
      sign( trx, monica_private_key );
      PUSH_TX( db, trx );
      trx.clear();

      // Proposal was executed
      BOOST_CHECK_EQUAL( 0, pidx.size() );
      const auto& content2 = db.get_content( "ipfs://abcdefg2" );
      BOOST_CHECK_EQUAL( 40, content2.manage_comp.weight_threshold );
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
