#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <muse/chain/database.hpp>
#include <muse/chain/exceptions.hpp>
#include <muse/chain/hardfork.hpp>
#include <muse/chain/history_object.hpp>
#include <muse/chain/base_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

#include <cmath>

using namespace muse::chain;
using namespace muse::chain::test;

BOOST_FIXTURE_TEST_SUITE( operation_time_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( comment_payout )
{
   try
   {
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );
      vest( "alice", 10000 );
      fund( "bob", 7500 );
      vest( "bob", 7500 );
      fund( "sam", 8000 );
      vest( "sam", 8000 );
      fund( "dave", 5000 );
      vest( "dave", 5000 );

      price exchange_rate( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) );
      set_price_feed( exchange_rate );

      signed_transaction tx;

      BOOST_TEST_MESSAGE( "Creating comments." );

      comment_operation com;
      com.author = "alice";
      com.permlink = "test";
      com.parent_author = "";
      com.parent_permlink = "test";
      com.title = "foo";
      com.body = "bar";
      tx.operations.push_back( com );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      com.author = "bob";
      tx.operations.push_back( com );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      BOOST_TEST_MESSAGE( "Voting for comments." );

      vote_operation vote;
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = MUSE_100_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      vote.voter = "sam";
      vote.author = "alice";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      vote.voter = "bob";
      vote.author = "bob";
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      vote.voter = "dave";
      vote.author = "bob";
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Generate blocks up until first payout" );

      //generate_blocks( db.get_comment( "bob", "test" ).cashout_time - MUSE_BLOCK_INTERVAL, true );

      auto reward_muse = db.get_dynamic_global_properties().total_reward_fund_muse + ASSET( "1.667 TESTS" );
      auto total_rshares2 = db.get_dynamic_global_properties().total_reward_shares2;
      auto bob_comment_rshares = db.get_comment( "bob", "test" ).net_rshares;
      auto bob_vest_shares = db.get_account( "bob" ).vesting_shares;
      auto bob_mbd_balance = db.get_account( "bob" ).mbd_balance;

      auto bob_comment_payout = asset( ( ( uint128_t( bob_comment_rshares.value ) * bob_comment_rshares.value * reward_muse.amount.value ) / total_rshares2 ).to_uint64(), MUSE_SYMBOL );
      auto bob_comment_discussion_rewards = asset( bob_comment_payout.amount / 4, MUSE_SYMBOL );
      bob_comment_payout -= bob_comment_discussion_rewards;
      auto bob_comment_mbd_reward = db.to_mbd( asset( bob_comment_payout.amount / 2, MUSE_SYMBOL ) );
      auto bob_comment_vesting_reward = ( bob_comment_payout - asset( bob_comment_payout.amount / 2, MUSE_SYMBOL) ) * db.get_dynamic_global_properties().get_vesting_share_price();

      BOOST_TEST_MESSAGE( "Cause first payout" );

      generate_block();

      BOOST_REQUIRE( db.get_dynamic_global_properties().total_reward_fund_muse == reward_muse - bob_comment_payout );
      BOOST_REQUIRE( db.get_comment( "bob", "test" ).total_payout_value == bob_comment_vesting_reward * db.get_dynamic_global_properties().get_vesting_share_price() + bob_comment_mbd_reward * exchange_rate );
      BOOST_REQUIRE( db.get_account( "bob" ).vesting_shares == bob_vest_shares + bob_comment_vesting_reward );
      BOOST_REQUIRE( db.get_account( "bob" ).mbd_balance == bob_mbd_balance + bob_comment_mbd_reward );

      BOOST_TEST_MESSAGE( "Testing no payout when less than $0.02" );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "alice";
      vote.author = "alice";
      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      vote.author = "bob";
      vote.weight = MUSE_1_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.get_comment( "bob", "test" ).cashout_time - MUSE_BLOCK_INTERVAL, true );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "bob";
      vote.author = "alice";
      vote.weight = MUSE_100_PERCENT;
      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "sam";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      bob_vest_shares = db.get_account( "bob" ).vesting_shares;
      bob_mbd_balance = db.get_account( "bob" ).mbd_balance;

      validate_database();

      generate_block();

      BOOST_REQUIRE_EQUAL( bob_vest_shares.amount.value, db.get_account( "bob" ).vesting_shares.amount.value );
      BOOST_REQUIRE_EQUAL( bob_mbd_balance.amount.value, db.get_account( "bob" ).mbd_balance.amount.value );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( discussion_rewards )
{

}

BOOST_AUTO_TEST_CASE( activity_rewards )
{

}

/*
BOOST_AUTO_TEST_CASE( comment_payout )
{
   try
   {
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );
      vest( "alice", 10000 );
      fund( "bob", 7500 );
      vest( "bob", 7500 );
      fund( "sam", 8000 );
      vest( "sam", 8000 );
      fund( "dave", 5000 );
      vest( "dave", 5000 );

      price exchange_rate( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) );
      set_price_feed( exchange_rate );

      auto gpo = db.get_dynamic_global_properties();

      signed_transaction tx;

      BOOST_TEST_MESSAGE( "Creating comments. " );

      comment_operation com;
      com.author = "alice";
      com.permlink = "test";
      com.parent_permlink = "test";
      com.title = "foo";
      com.body = "bar";
      tx.operations.push_back( com );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      com.author = "bob";
      tx.operations.push_back( com );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "First round of votes." );

      tx.operations.clear();
      tx.signatures.clear();
      vote_operation vote;
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = MUSE_100_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "bob";
      vote.author = "bob";
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "sam";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Generating blocks..." );

      generate_blocks( fc::time_point_sec( db.head_block_time().sec_since_epoch() + MUSE_CASHOUT_WINDOW_SECONDS / 2 ), true );

      BOOST_TEST_MESSAGE( "Second round of votes." );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "alice";
      vote.author = "bob";
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "bob";
      vote.author = "alice";
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "sam";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Generating more blocks..." );

      generate_blocks( db.get_comment( "bob", "test" ).cashout_time - ( MUSE_BLOCK_INTERVAL / 2 ), true );

      BOOST_TEST_MESSAGE( "Check comments have not been paid out." );

      const auto& vote_idx = db.get_index_type< comment_vote_index >().indices().get< by_comment_voter >();

      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "alice" ).id ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "bob" ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "sam" ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "dave" ).id  ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id,   db.get_account( "alice" ).id ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id,   db.get_account( "bob" ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id,   db.get_account( "sam" ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).net_rshares.value > 0 );
      BOOST_REQUIRE( db.get_comment( "bob", "test" ).net_rshares.value > 0 );
      validate_database();

      auto reward_muse = db.get_dynamic_global_properties().total_reward_fund_muse + ASSET( "2.000 TESTS" );
      auto total_rshares2 = db.get_dynamic_global_properties().total_reward_shares2;
      auto bob_comment_vote_total = db.get_comment( "bob", "test" ).total_vote_weight;
      auto bob_comment_rshares = db.get_comment( "bob", "test" ).net_rshares;
      auto bob_mbd_balance = db.get_account( "bob" ).mbd_balance;
      auto alice_vest_shares = db.get_account( "alice" ).vesting_shares;
      auto bob_vest_shares = db.get_account( "bob" ).vesting_shares;
      auto sam_vest_shares = db.get_account( "sam" ).vesting_shares;
      auto dave_vest_shares = db.get_account( "dave" ).vesting_shares;

      auto bob_comment_payout = asset( ( ( uint128_t( bob_comment_rshares.value ) * bob_comment_rshares.value * reward_muse.amount.value ) / total_rshares2 ).to_uint64(), MUSE_SYMBOL );
      auto bob_comment_vote_rewards = asset( bob_comment_payout.amount / 2, MUSE_SYMBOL );
      bob_comment_payout -= bob_comment_vote_rewards;
      auto bob_comment_mbd_reward = asset( bob_comment_payout.amount / 2, MUSE_SYMBOL ) * exchange_rate;
      auto bob_comment_vesting_reward = ( bob_comment_payout - asset( bob_comment_payout.amount / 2, MUSE_SYMBOL ) ) * db.get_dynamic_global_properties().get_vesting_share_price();
      auto unclaimed_payments = bob_comment_vote_rewards;
      auto alice_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id, db.get_account( "alice" ).id ) )->weight ) * bob_comment_vote_rewards.amount.value ) / bob_comment_vote_total ), MUSE_SYMBOL );
      auto alice_vote_vesting = alice_vote_reward * db.get_dynamic_global_properties().get_vesting_share_price();
      auto bob_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id, db.get_account( "bob" ).id ) )->weight ) * bob_comment_vote_rewards.amount.value ) / bob_comment_vote_total ), MUSE_SYMBOL );
      auto bob_vote_vesting = bob_vote_reward * db.get_dynamic_global_properties().get_vesting_share_price();
      auto sam_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id, db.get_account( "sam" ).id ) )->weight ) * bob_comment_vote_rewards.amount.value ) / bob_comment_vote_total ), MUSE_SYMBOL );
      auto sam_vote_vesting = sam_vote_reward * db.get_dynamic_global_properties().get_vesting_share_price();
      unclaimed_payments -= ( alice_vote_reward + bob_vote_reward + sam_vote_reward );

      BOOST_TEST_MESSAGE( "Generate one block to cause a payout" );

      generate_block();

      auto bob_comment_reward = get_last_operations( 1 )[0].get< comment_reward_operation >();

      BOOST_REQUIRE_EQUAL( db.get_dynamic_global_properties().total_reward_fund_muse.amount.value, reward_muse.amount.value - ( bob_comment_payout + bob_comment_vote_rewards - unclaimed_payments ).amount.value );
      BOOST_REQUIRE_EQUAL( db.get_comment( "bob", "test" ).total_payout_value.amount.value, ( ( bob_comment_vesting_reward * db.get_dynamic_global_properties().get_vesting_share_price() ) + ( bob_comment_mbd_reward * exchange_rate ) ).amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "bob" ).mbd_balance.amount.value, ( bob_mbd_balance + bob_comment_mbd_reward ).amount.value );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).net_rshares.value > 0 );
      BOOST_REQUIRE( db.get_comment( "bob", "test" ).net_rshares.value == 0 );
      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).vesting_shares.amount.value, ( alice_vest_shares + alice_vote_vesting ).amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "bob" ).vesting_shares.amount.value, ( bob_vest_shares + bob_vote_vesting + bob_comment_vesting_reward ).amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "sam" ).vesting_shares.amount.value, ( sam_vest_shares + sam_vote_vesting ).amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "dave" ).vesting_shares.amount.value, dave_vest_shares.amount.value );
      BOOST_REQUIRE_EQUAL( bob_comment_reward.author, "bob" );
      BOOST_REQUIRE_EQUAL( bob_comment_reward.permlink, "test" );
      BOOST_REQUIRE_EQUAL( bob_comment_reward.payout.amount.value, bob_comment_mbd_reward.amount.value );
      BOOST_REQUIRE_EQUAL( bob_comment_reward.vesting_payout.amount.value, bob_comment_vesting_reward.amount.value );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "alice" ).id ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "bob" ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "sam" ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "dave" ).id  ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id,   db.get_account( "alice" ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id,   db.get_account( "bob" ).id   ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id,   db.get_account( "sam" ).id   ) ) == vote_idx.end() );
      validate_database();

      BOOST_TEST_MESSAGE( "Generating blocks up to next comment payout" );

      generate_blocks( db.get_comment( "alice", "test" ).cashout_time - ( MUSE_BLOCK_INTERVAL / 2 ), true );

      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "alice" ).id ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "bob" ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "sam" ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "dave" ).id  ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id,   db.get_account( "alice" ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id,   db.get_account( "bob" ).id   ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id,   db.get_account( "sam" ).id   ) ) == vote_idx.end() );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).net_rshares.value > 0 );
      BOOST_REQUIRE( db.get_comment( "bob", "test" ).net_rshares.value == 0 );
      validate_database();

      BOOST_TEST_MESSAGE( "Generate block to cause payout" );

      reward_muse = db.get_dynamic_global_properties().total_reward_fund_muse + ASSET( "2.000 TESTS" );
      total_rshares2 = db.get_dynamic_global_properties().total_reward_shares2;
      auto alice_comment_vote_total = db.get_comment( "alice", "test" ).total_vote_weight;
      auto alice_comment_rshares = db.get_comment( "alice", "test" ).net_rshares;
      auto alice_mbd_balance = db.get_account( "alice" ).mbd_balance;
      alice_vest_shares = db.get_account( "alice" ).vesting_shares;
      bob_vest_shares = db.get_account( "bob" ).vesting_shares;
      sam_vest_shares = db.get_account( "sam" ).vesting_shares;
      dave_vest_shares = db.get_account( "dave" ).vesting_shares;

      u256 rs( alice_comment_rshares.value );
      u256 rf( reward_muse.amount.value );
      u256 trs2 = total_rshares2.hi;
      trs2 = ( trs2 << 64 ) + total_rshares2.lo;
      auto rs2 = rs*rs;

      auto alice_comment_payout = asset( static_cast< uint64_t >( ( rf * rs2 ) / trs2 ), MUSE_SYMBOL );
      auto alice_comment_vote_rewards = asset( alice_comment_payout.amount / 2, MUSE_SYMBOL );
      alice_comment_payout -= alice_comment_vote_rewards;
      auto alice_comment_mbd_reward = asset( alice_comment_payout.amount / 2, MUSE_SYMBOL ) * exchange_rate;
      auto alice_comment_vesting_reward = ( alice_comment_payout - asset( alice_comment_payout.amount / 2, MUSE_SYMBOL ) ) * db.get_dynamic_global_properties().get_vesting_share_price();
      unclaimed_payments = alice_comment_vote_rewards;
      alice_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "alice" ).id ) )->weight ) * alice_comment_vote_rewards.amount.value ) / alice_comment_vote_total ), MUSE_SYMBOL );
      alice_vote_vesting = alice_vote_reward * db.get_dynamic_global_properties().get_vesting_share_price();
      bob_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "bob" ).id ) )->weight ) * alice_comment_vote_rewards.amount.value ) / alice_comment_vote_total ), MUSE_SYMBOL );
      bob_vote_vesting = bob_vote_reward * db.get_dynamic_global_properties().get_vesting_share_price();
      sam_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "sam" ).id ) )->weight ) * alice_comment_vote_rewards.amount.value ) / alice_comment_vote_total ), MUSE_SYMBOL );
      sam_vote_vesting = sam_vote_reward * db.get_dynamic_global_properties().get_vesting_share_price();
      auto dave_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "dave" ).id ) )->weight ) * alice_comment_vote_rewards.amount.value ) / alice_comment_vote_total ), MUSE_SYMBOL );
      auto dave_vote_vesting = dave_vote_reward * db.get_dynamic_global_properties().get_vesting_share_price();
      unclaimed_payments -= ( alice_vote_reward + bob_vote_reward + sam_vote_reward + dave_vote_reward );

      generate_block();
      auto alice_comment_reward = get_last_operations( 1 )[0].get< comment_reward_operation >();

      BOOST_REQUIRE_EQUAL( ( db.get_dynamic_global_properties().total_reward_fund_muse + alice_comment_payout + alice_comment_vote_rewards - unclaimed_payments ).amount.value, reward_muse.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_comment( "alice", "test" ).total_payout_value.amount.value, ( ( alice_comment_vesting_reward * db.get_dynamic_global_properties().get_vesting_share_price() ) + ( alice_comment_mbd_reward * exchange_rate ) ).amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).mbd_balance.amount.value, ( alice_mbd_balance + alice_comment_mbd_reward ).amount.value );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).net_rshares.value == 0 );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).net_rshares.value == 0 );
      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).vesting_shares.amount.value, ( alice_vest_shares + alice_vote_vesting + alice_comment_vesting_reward ).amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "bob" ).vesting_shares.amount.value, ( bob_vest_shares + bob_vote_vesting ).amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "sam" ).vesting_shares.amount.value, ( sam_vest_shares + sam_vote_vesting ).amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "dave" ).vesting_shares.amount.value, ( dave_vest_shares + dave_vote_vesting ).amount.value );
      BOOST_REQUIRE_EQUAL( alice_comment_reward.author, "alice" );
      BOOST_REQUIRE_EQUAL( alice_comment_reward.permlink, "test" );
      BOOST_REQUIRE_EQUAL( alice_comment_reward.payout.amount.value, alice_comment_mbd_reward.amount.value );
      BOOST_REQUIRE_EQUAL( alice_comment_reward.vesting_payout.amount.value, alice_comment_vesting_reward.amount.value );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "alice" ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "bob" ).id   ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "sam" ).id   ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "dave" ).id  ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id,   db.get_account( "alice" ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id,   db.get_account( "bob" ).id   ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id,   db.get_account( "sam" ).id   ) ) == vote_idx.end() );
      validate_database();

      BOOST_TEST_MESSAGE( "Testing no payout when less than $0.02" );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "alice";
      vote.author = "alice";
      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      vote.author = "bob";
      vote.weight = MUSE_1_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.get_comment( "bob", "test" ).cashout_time - MUSE_BLOCK_INTERVAL, true );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "bob";
      vote.author = "alice";
      vote.weight = MUSE_100_PERCENT;
      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "sam";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      bob_vest_shares = db.get_account( "bob" ).vesting_shares;
      auto bob_mbd = db.get_account( "bob" ).mbd_balance;

      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id, db.get_account( "dave" ).id ) ) != vote_idx.end() );
      validate_database();

      generate_block();

      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id, db.get_account( "dave" ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE_EQUAL( bob_vest_shares.amount.value, db.get_account( "bob" ).vesting_shares.amount.value );
      BOOST_REQUIRE_EQUAL( bob_mbd.amount.value, db.get_account( "bob" ).mbd_balance.amount.value );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( nested_comments )
{
   try
   {
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );
      vest( "alice", 10000 );
      fund( "bob", 10000 );
      vest( "bob", 10000 );
      fund( "sam", 10000 );
      vest( "sam", 10000 );
      fund( "dave", 10000 );
      vest( "dave", 10000 );

      price exchange_rate( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) );
      set_price_feed( exchange_rate );

      signed_transaction tx;
      comment_operation comment_op;
      comment_op.author = "alice";
      comment_op.permlink = "test";
      comment_op.parent_permlink = "test";
      comment_op.title = "foo";
      comment_op.body = "bar";
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( comment_op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      comment_op.author = "bob";
      comment_op.parent_author = "alice";
      comment_op.parent_permlink = "test";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( comment_op );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      comment_op.author = "sam";
      comment_op.parent_author = "bob";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( comment_op );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      comment_op.author = "dave";
      comment_op.parent_author = "sam";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( comment_op );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      vote_operation vote_op;
      vote_op.voter = "alice";
      vote_op.author = "alice";
      vote_op.permlink = "test";
      vote_op.weight = MUSE_100_PERCENT;
      tx.operations.push_back( vote_op );
      vote_op.author = "bob";
      tx.operations.push_back( vote_op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote_op.voter = "bob";
      vote_op.author = "alice";
      tx.operations.push_back( vote_op );
      vote_op.author = "bob";
      tx.operations.push_back( vote_op );
      vote_op.author = "dave";
      vote_op.weight = MUSE_1_PERCENT * 20;
      tx.operations.push_back( vote_op );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote_op.voter = "sam";
      vote_op.author = "bob";
      vote_op.weight = MUSE_100_PERCENT;
      tx.operations.push_back( vote_op );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.get_comment( "alice", "test" ).cashout_time - fc::seconds( MUSE_BLOCK_INTERVAL ), true );

      auto gpo = db.get_dynamic_global_properties();
      uint128_t reward_muse = gpo.total_reward_fund_muse.amount.value + ASSET( "2.000 TESTS" ).amount.value;
      uint128_t total_rshares2 = gpo.total_reward_shares2;

      auto alice_comment = db.get_comment( "alice", "test" );
      auto bob_comment = db.get_comment( "bob", "test" );
      auto sam_comment = db.get_comment( "sam", "test" );
      auto dave_comment = db.get_comment( "dave", "test" );

      const auto& vote_idx = db.get_index_type< comment_vote_index >().indices().get< by_comment_voter >();

      // Calculate total comment rewards and voting rewards.
      auto alice_comment_reward = ( ( reward_muse * alice_comment.net_rshares.value * alice_comment.net_rshares.value ) / total_rshares2 ).to_uint64();
      total_rshares2 -= uint128_t( alice_comment.net_rshares.value ) * ( alice_comment.net_rshares.value );
      reward_muse -= alice_comment_reward;
      auto alice_comment_vote_rewards = alice_comment_reward / 2;
      alice_comment_reward -= alice_comment_vote_rewards;

      auto alice_vote_alice_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "alice" ).id ) )->weight ) * alice_comment_vote_rewards ) / alice_comment.total_vote_weight ), MUSE_SYMBOL );
      auto bob_vote_alice_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "alice", "test" ).id, db.get_account( "bob" ).id ) )->weight ) * alice_comment_vote_rewards ) / alice_comment.total_vote_weight ), MUSE_SYMBOL );
      reward_muse += alice_comment_vote_rewards - ( alice_vote_alice_reward + bob_vote_alice_reward ).amount.value;

      auto bob_comment_reward = ( ( reward_muse * bob_comment.net_rshares.value * bob_comment.net_rshares.value ) / total_rshares2 ).to_uint64();
      total_rshares2 -= uint128_t( bob_comment.net_rshares.value ) * bob_comment.net_rshares.value;
      reward_muse -= bob_comment_reward;
      auto bob_comment_vote_rewards = bob_comment_reward / 2;
      bob_comment_reward -= bob_comment_vote_rewards;

      auto alice_vote_bob_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id, db.get_account( "alice" ).id ) )->weight ) * bob_comment_vote_rewards ) / bob_comment.total_vote_weight ), MUSE_SYMBOL );
      auto bob_vote_bob_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id, db.get_account( "bob" ).id ) )->weight ) * bob_comment_vote_rewards ) / bob_comment.total_vote_weight ), MUSE_SYMBOL );
      auto sam_vote_bob_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "bob", "test" ).id, db.get_account( "sam" ).id ) )->weight ) * bob_comment_vote_rewards ) / bob_comment.total_vote_weight ), MUSE_SYMBOL );
      reward_muse += bob_comment_vote_rewards - ( alice_vote_bob_reward + bob_vote_bob_reward + sam_vote_bob_reward ).amount.value;

      auto dave_comment_reward = ( ( reward_muse * dave_comment.net_rshares.value * dave_comment.net_rshares.value ) / total_rshares2 ).to_uint64();
      total_rshares2 -= uint128_t( dave_comment.net_rshares.value ) * dave_comment.net_rshares.value;
      reward_muse -= dave_comment_reward;
      auto dave_comment_vote_rewards = dave_comment_reward / 2;
      dave_comment_reward -= dave_comment_vote_rewards;

      auto bob_vote_dave_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "dave", "test" ).id, db.get_account( "bob" ).id ) )->weight ) * dave_comment_vote_rewards ) / dave_comment.total_vote_weight ), MUSE_SYMBOL );
      reward_muse += dave_comment_vote_rewards - bob_vote_dave_reward.amount.value;

      // Calculate rewards paid to parent posts
      auto alice_pays_alice_mbd = alice_comment_reward / 2;
      auto alice_pays_alice_vest = alice_comment_reward - alice_pays_alice_mbd;
      auto bob_pays_bob_mbd = bob_comment_reward / 2;
      auto bob_pays_bob_vest = bob_comment_reward - bob_pays_bob_mbd;
      auto dave_pays_dave_mbd = dave_comment_reward / 2;
      auto dave_pays_dave_vest = dave_comment_reward - dave_pays_dave_mbd;

      auto bob_pays_alice_mbd = bob_pays_bob_mbd / 2;
      auto bob_pays_alice_vest = bob_pays_bob_vest / 2;
      bob_pays_bob_mbd -= bob_pays_alice_mbd;
      bob_pays_bob_vest -= bob_pays_alice_vest;

      auto dave_pays_sam_mbd = dave_pays_dave_mbd / 2;
      auto dave_pays_sam_vest = dave_pays_dave_vest / 2;
      dave_pays_dave_mbd -= dave_pays_sam_mbd;
      dave_pays_dave_vest -= dave_pays_sam_vest;
      auto dave_pays_bob_mbd = dave_pays_sam_mbd / 2;
      auto dave_pays_bob_vest = dave_pays_sam_vest / 2;
      dave_pays_sam_mbd -= dave_pays_bob_mbd;
      dave_pays_sam_vest -= dave_pays_bob_vest;
      auto dave_pays_alice_mbd = dave_pays_bob_mbd / 2;
      auto dave_pays_alice_vest = dave_pays_bob_vest / 2;
      dave_pays_bob_mbd -= dave_pays_alice_mbd;
      dave_pays_bob_vest -= dave_pays_alice_vest;

      // Calculate total comment payouts
      auto alice_comment_total_payout = db.to_mbd( asset( alice_pays_alice_mbd + alice_pays_alice_vest, MUSE_SYMBOL ) );
      alice_comment_total_payout += db.to_mbd( asset( bob_pays_alice_mbd + bob_pays_alice_vest, MUSE_SYMBOL ) );
      alice_comment_total_payout += db.to_mbd( asset( dave_pays_alice_mbd + dave_pays_alice_vest, MUSE_SYMBOL ) );
      auto bob_comment_total_payout = db.to_mbd( asset( bob_pays_bob_mbd + bob_pays_bob_vest, MUSE_SYMBOL ) );
      bob_comment_total_payout += db.to_mbd( asset( dave_pays_bob_mbd + dave_pays_bob_vest, MUSE_SYMBOL ) );
      auto sam_comment_total_payout = db.to_mbd( asset( dave_pays_sam_mbd + dave_pays_sam_vest, MUSE_SYMBOL ) );
      auto dave_comment_total_payout = db.to_mbd( asset( dave_pays_dave_mbd + dave_pays_dave_vest, MUSE_SYMBOL ) );

      auto alice_starting_vesting = db.get_account( "alice" ).vesting_shares;
      auto alice_starting_mbd = db.get_account( "alice" ).mbd_balance;
      auto bob_starting_vesting = db.get_account( "bob" ).vesting_shares;
      auto bob_starting_mbd = db.get_account( "bob" ).mbd_balance;
      auto sam_starting_vesting = db.get_account( "sam" ).vesting_shares;
      auto sam_starting_mbd = db.get_account( "sam" ).mbd_balance;
      auto dave_starting_vesting = db.get_account( "dave" ).vesting_shares;
      auto dave_starting_mbd = db.get_account( "dave" ).mbd_balance;

      generate_block();

      gpo = db.get_dynamic_global_properties();

      // Calculate vesting share rewards from voting.
      auto alice_vote_alice_vesting = alice_vote_alice_reward * gpo.get_vesting_share_price();
      auto bob_vote_alice_vesting = bob_vote_alice_reward * gpo.get_vesting_share_price();
      auto alice_vote_bob_vesting = alice_vote_bob_reward * gpo.get_vesting_share_price();
      auto bob_vote_bob_vesting = bob_vote_bob_reward * gpo.get_vesting_share_price();
      auto sam_vote_bob_vesting = sam_vote_bob_reward * gpo.get_vesting_share_price();
      auto bob_vote_dave_vesting = bob_vote_dave_reward * gpo.get_vesting_share_price();

      BOOST_REQUIRE_EQUAL( db.get_comment( "alice", "test" ).total_payout_value.amount.value, alice_comment_total_payout.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_comment( "bob", "test" ).total_payout_value.amount.value, bob_comment_total_payout.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_comment( "sam", "test" ).total_payout_value.amount.value, sam_comment_total_payout.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_comment( "dave", "test" ).total_payout_value.amount.value, dave_comment_total_payout.amount.value );

      // ops 0-3, 5-6, and 10 are comment reward ops
      auto ops = get_last_operations( 13 );

      BOOST_TEST_MESSAGE( "Checking Virtual Operation Correctness" );

      curate_reward_operation cur_vop;
      comment_reward_operation com_vop = ops[0].get< comment_reward_operation >();

      BOOST_REQUIRE_EQUAL( com_vop.author, "alice" );
      BOOST_REQUIRE_EQUAL( com_vop.permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_author, "dave" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.payout.amount.value, dave_pays_alice_mbd );
      BOOST_REQUIRE_EQUAL( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value, dave_pays_alice_vest );

      com_vop = ops[1].get< comment_reward_operation >();
      BOOST_REQUIRE_EQUAL( com_vop.author, "bob" );
      BOOST_REQUIRE_EQUAL( com_vop.permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_author, "dave" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.payout.amount.value, dave_pays_bob_mbd );
      BOOST_REQUIRE_EQUAL( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value, dave_pays_bob_vest );

      com_vop = ops[2].get< comment_reward_operation >();
      BOOST_REQUIRE_EQUAL( com_vop.author, "sam" );
      BOOST_REQUIRE_EQUAL( com_vop.permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_author, "dave" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.payout.amount.value, dave_pays_sam_mbd );
      BOOST_REQUIRE_EQUAL( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value, dave_pays_sam_vest );

      com_vop = ops[3].get< comment_reward_operation >();
      BOOST_REQUIRE_EQUAL( com_vop.author, "dave" );
      BOOST_REQUIRE_EQUAL( com_vop.permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_author, "dave" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.payout.amount.value, dave_pays_dave_mbd );
      BOOST_REQUIRE_EQUAL( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value, dave_pays_dave_vest );

      cur_vop = ops[4].get< curate_reward_operation >();
      BOOST_REQUIRE_EQUAL( cur_vop.curator, "bob" );
      BOOST_REQUIRE_EQUAL( cur_vop.reward.amount.value, bob_vote_dave_vesting.amount.value );
      BOOST_REQUIRE_EQUAL( cur_vop.comment_author, "dave" );
      BOOST_REQUIRE_EQUAL( cur_vop.comment_permlink, "test" );

      com_vop = ops[5].get< comment_reward_operation >();
      BOOST_REQUIRE_EQUAL( com_vop.author, "alice" );
      BOOST_REQUIRE_EQUAL( com_vop.permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_author, "bob" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.payout.amount.value, bob_pays_alice_mbd );
      BOOST_REQUIRE_EQUAL( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value, bob_pays_alice_vest );

      com_vop = ops[6].get< comment_reward_operation >();
      BOOST_REQUIRE_EQUAL( com_vop.author, "bob" );
      BOOST_REQUIRE_EQUAL( com_vop.permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_author, "bob" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.payout.amount.value, bob_pays_bob_mbd );
      BOOST_REQUIRE_EQUAL( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value, bob_pays_bob_vest );

      cur_vop = ops[7].get< curate_reward_operation >();
      BOOST_REQUIRE_EQUAL( cur_vop.curator, "sam" );
      BOOST_REQUIRE_EQUAL( cur_vop.reward.amount.value, sam_vote_bob_vesting.amount.value );
      BOOST_REQUIRE_EQUAL( cur_vop.comment_author, "bob" );
      BOOST_REQUIRE_EQUAL( cur_vop.comment_permlink, "test" );

      cur_vop = ops[8].get< curate_reward_operation >();
      BOOST_REQUIRE_EQUAL( cur_vop.curator, "bob" );
      BOOST_REQUIRE_EQUAL( cur_vop.reward.amount.value, bob_vote_bob_vesting.amount.value );
      BOOST_REQUIRE_EQUAL( cur_vop.comment_author, "bob" );
      BOOST_REQUIRE_EQUAL( cur_vop.comment_permlink, "test" );

      cur_vop = ops[9].get< curate_reward_operation >();
      BOOST_REQUIRE_EQUAL( cur_vop.curator, "alice" );
      BOOST_REQUIRE_EQUAL( cur_vop.reward.amount.value, alice_vote_bob_vesting.amount.value );
      BOOST_REQUIRE_EQUAL( cur_vop.comment_author, "bob" );
      BOOST_REQUIRE_EQUAL( cur_vop.comment_permlink, "test" );

      com_vop = ops[10].get< comment_reward_operation >();
      BOOST_REQUIRE_EQUAL( com_vop.author, "alice" );
      BOOST_REQUIRE_EQUAL( com_vop.permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_author, "alice" );
      BOOST_REQUIRE_EQUAL( com_vop.originating_permlink, "test" );
      BOOST_REQUIRE_EQUAL( com_vop.payout.amount.value, alice_pays_alice_mbd );
      BOOST_REQUIRE_EQUAL( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value, alice_pays_alice_vest );

      cur_vop = ops[11].get< curate_reward_operation >();
      BOOST_REQUIRE_EQUAL( cur_vop.curator, "bob" );
      BOOST_REQUIRE_EQUAL( cur_vop.reward.amount.value, bob_vote_alice_vesting.amount.value );
      BOOST_REQUIRE_EQUAL( cur_vop.comment_author, "alice" );
      BOOST_REQUIRE_EQUAL( cur_vop.comment_permlink, "test" );

      cur_vop = ops[12].get< curate_reward_operation >();
      BOOST_REQUIRE_EQUAL( cur_vop.curator, "alice" );
      BOOST_REQUIRE_EQUAL( cur_vop.reward.amount.value, alice_vote_alice_vesting.amount.value );
      BOOST_REQUIRE_EQUAL( cur_vop.comment_author, "alice" );
      BOOST_REQUIRE_EQUAL( cur_vop.comment_permlink, "test" );

      BOOST_TEST_MESSAGE( "Checking account balances" );

      auto alice_total_mbd = alice_starting_mbd + asset( alice_pays_alice_mbd + bob_pays_alice_mbd + dave_pays_alice_mbd, MUSE_SYMBOL ) * exchange_rate;
      auto alice_total_vesting = alice_starting_vesting + asset( alice_pays_alice_vest + bob_pays_alice_vest + dave_pays_alice_vest + alice_vote_alice_reward.amount + alice_vote_bob_reward.amount, MUSE_SYMBOL ) * gpo.get_vesting_share_price();
      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).mbd_balance.amount.value, alice_total_mbd.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).vesting_shares.amount.value, alice_total_vesting.amount.value );

      auto bob_total_mbd = bob_starting_mbd + asset( bob_pays_bob_mbd + dave_pays_bob_mbd, MUSE_SYMBOL ) * exchange_rate;
      auto bob_total_vesting = bob_starting_vesting + asset( bob_pays_bob_vest + dave_pays_bob_vest + bob_vote_alice_reward.amount + bob_vote_bob_reward.amount + bob_vote_dave_reward.amount, MUSE_SYMBOL ) * gpo.get_vesting_share_price();
      BOOST_REQUIRE_EQUAL( db.get_account( "bob" ).mbd_balance.amount.value, bob_total_mbd.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "bob" ).vesting_shares.amount.value, bob_total_vesting.amount.value );

      auto sam_total_mbd = sam_starting_mbd + asset( dave_pays_sam_mbd, MUSE_SYMBOL ) * exchange_rate;
      auto sam_total_vesting = bob_starting_vesting + asset( dave_pays_sam_vest + sam_vote_bob_reward.amount, MUSE_SYMBOL ) * gpo.get_vesting_share_price();
      BOOST_REQUIRE_EQUAL( db.get_account( "sam" ).mbd_balance.amount.value, sam_total_mbd.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "sam" ).vesting_shares.amount.value, sam_total_vesting.amount.value );

      auto dave_total_mbd = dave_starting_mbd + asset( dave_pays_dave_mbd, MUSE_SYMBOL ) * exchange_rate;
      auto dave_total_vesting = dave_starting_vesting + asset( dave_pays_dave_vest, MUSE_SYMBOL ) * gpo.get_vesting_share_price();
      BOOST_REQUIRE_EQUAL( db.get_account( "dave" ).mbd_balance.amount.value, dave_total_mbd.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "dave" ).vesting_shares.amount.value, dave_total_vesting.amount.value );
   }
   FC_LOG_AND_RETHROW()
}
*/
BOOST_AUTO_TEST_CASE( vesting_withdrawals )
{
   try
   {
      ACTORS( (alice) )
      fund( "alice", 100000 );
      vest( "alice", 100000 );

      const auto& new_alice = db.get_account( "alice" );

      BOOST_TEST_MESSAGE( "Setting up withdrawal" );

      signed_transaction tx;
      withdraw_vesting_operation op;
      op.account = "alice";
      op.vesting_shares = asset( new_alice.vesting_shares.amount / 2, VESTS_SYMBOL );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      auto next_withdrawal = db.head_block_time() + MUSE_VESTING_WITHDRAW_INTERVAL_SECONDS;
      asset vesting_shares = new_alice.vesting_shares;
      asset original_vesting = vesting_shares;
      asset withdraw_rate = new_alice.vesting_withdraw_rate;

      BOOST_TEST_MESSAGE( "Generating block up to first withdrawal" );
      generate_blocks( next_withdrawal - ( MUSE_BLOCK_INTERVAL / 2 ), true);

      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).vesting_shares.amount.value, vesting_shares.amount.value );

      BOOST_TEST_MESSAGE( "Generating block to cause withdrawal" );
      generate_block();

      auto fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
      auto gpo = db.get_dynamic_global_properties();

      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).vesting_shares.amount.value, ( vesting_shares - withdraw_rate ).amount.value );
      BOOST_REQUIRE( ( withdraw_rate * gpo.get_vesting_share_price() ).amount.value - db.get_account( "alice" ).balance.amount.value <= 1 ); // Check a range due to differences in the share price
      BOOST_REQUIRE_EQUAL( fill_op.from_account, "alice" );
      BOOST_REQUIRE_EQUAL( fill_op.to_account, "alice" );
      BOOST_REQUIRE_EQUAL( fill_op.withdrawn.amount.value, withdraw_rate.amount.value );
      BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );
      validate_database();

      BOOST_TEST_MESSAGE( "Generating the rest of the blocks in the withdrawal" );

      vesting_shares = db.get_account( "alice" ).vesting_shares;
      auto balance = db.get_account( "alice" ).balance;
      auto old_next_vesting = db.get_account( "alice" ).next_vesting_withdrawal;

      for( int i = 1; i < MUSE_VESTING_WITHDRAW_INTERVALS - 1; i++ )
      {
         generate_blocks( db.head_block_time() + MUSE_VESTING_WITHDRAW_INTERVAL_SECONDS );

         const auto& alice = db.get_account( "alice" );

         gpo = db.get_dynamic_global_properties();
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();

         BOOST_REQUIRE_EQUAL( alice.vesting_shares.amount.value, ( vesting_shares - withdraw_rate ).amount.value );
         BOOST_REQUIRE( balance.amount.value + ( withdraw_rate * gpo.get_vesting_share_price() ).amount.value - alice.balance.amount.value <= 1 );
         BOOST_REQUIRE_EQUAL( fill_op.from_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.to_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.withdrawn.amount.value, withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );

         if ( i == MUSE_VESTING_WITHDRAW_INTERVALS - 1 )
            BOOST_REQUIRE( alice.next_vesting_withdrawal == fc::time_point_sec::maximum() );
         else
            BOOST_REQUIRE_EQUAL( alice.next_vesting_withdrawal.sec_since_epoch(), ( old_next_vesting + MUSE_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );

         validate_database();

         vesting_shares = alice.vesting_shares;
         balance = alice.balance;
         old_next_vesting = alice.next_vesting_withdrawal;
      }

      if (  original_vesting.amount.value % withdraw_rate.amount.value != 0 )
      {
         BOOST_TEST_MESSAGE( "Generating one more block to take care of remainder" );
         generate_blocks( db.head_block_time() + MUSE_VESTING_WITHDRAW_INTERVAL_SECONDS, true );
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();

         BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch(), ( old_next_vesting + MUSE_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );
         BOOST_REQUIRE_EQUAL( fill_op.from_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.to_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.withdrawn.amount.value, withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );

         generate_blocks( db.head_block_time() + MUSE_VESTING_WITHDRAW_INTERVAL_SECONDS, true );
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();

         BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch(), ( old_next_vesting + MUSE_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );
         BOOST_REQUIRE_EQUAL( fill_op.to_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.from_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.withdrawn.amount.value, original_vesting.amount.value % withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );

         validate_database();
      }
      else
      {
         generate_blocks( db.head_block_time() + MUSE_VESTING_WITHDRAW_INTERVAL_SECONDS, true );

         BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch(), fc::time_point_sec::maximum().sec_since_epoch() );

         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
         BOOST_REQUIRE_EQUAL( fill_op.from_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.to_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.withdrawn.amount.value, withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );
      }

      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).vesting_shares.amount.value, ( original_vesting - op.vesting_shares ).amount.value );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( vesting_withdraw_route )
{
   try
   {
      ACTORS( (alice)(bob)(sam) )

      auto original_vesting = alice.vesting_shares;

      fund( "alice", 1040000 );
      vest( "alice", 1040000 );

      auto withdraw_amount = alice.vesting_shares - original_vesting;

      BOOST_TEST_MESSAGE( "Setup vesting withdraw" );
      withdraw_vesting_operation wv;
      wv.account = "alice";
      wv.vesting_shares = withdraw_amount;

      signed_transaction tx;
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( wv );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      BOOST_TEST_MESSAGE( "Setting up bob destination" );
      set_withdraw_vesting_route_operation op;
      op.from_account = "alice";
      op.to_account = "bob";
      op.percent = MUSE_1_PERCENT * 50;
      op.auto_vest = true;
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "Setting up sam destination" );
      op.to_account = "sam";
      op.percent = MUSE_1_PERCENT * 30;
      op.auto_vest = false;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Setting up first withdraw" );

      auto vesting_withdraw_rate = alice.vesting_withdraw_rate;
      auto old_alice_balance = alice.balance;
      auto old_alice_vesting = alice.vesting_shares;
      auto old_bob_balance = bob.balance;
      auto old_bob_vesting = bob.vesting_shares;
      auto old_sam_balance = sam.balance;
      auto old_sam_vesting = sam.vesting_shares;
      generate_blocks( alice.next_vesting_withdrawal, true );

      {
         const auto& alice = db.get_account( "alice" );
         const auto& bob = db.get_account( "bob" );
         const auto& sam = db.get_account( "sam" );

         BOOST_REQUIRE( alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate );
         BOOST_REQUIRE( alice.balance == old_alice_balance + asset( ( vesting_withdraw_rate.amount * MUSE_1_PERCENT * 20 ) / MUSE_100_PERCENT, VESTS_SYMBOL ) * db.get_dynamic_global_properties().get_vesting_share_price() );
         BOOST_REQUIRE( bob.vesting_shares == old_bob_vesting + asset( ( vesting_withdraw_rate.amount * MUSE_1_PERCENT * 50 ) / MUSE_100_PERCENT, VESTS_SYMBOL ) );
         BOOST_REQUIRE( bob.balance == old_bob_balance );
         BOOST_REQUIRE( sam.vesting_shares == old_sam_vesting );
         BOOST_REQUIRE( sam.balance ==  old_sam_balance + asset( ( vesting_withdraw_rate.amount * MUSE_1_PERCENT * 30 ) / MUSE_100_PERCENT, VESTS_SYMBOL ) * db.get_dynamic_global_properties().get_vesting_share_price() );

         old_alice_balance = alice.balance;
         old_alice_vesting = alice.vesting_shares;
         old_bob_balance = bob.balance;
         old_bob_vesting = bob.vesting_shares;
         old_sam_balance = sam.balance;
         old_sam_vesting = sam.vesting_shares;
      }

      BOOST_TEST_MESSAGE( "Test failure with greater than 100% destination assignment" );

      tx.operations.clear();
      tx.signatures.clear();

      op.to_account = "sam";
      op.percent = MUSE_1_PERCENT * 50 + 1;
      tx.operations.push_back( op );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), fc::assert_exception );

      BOOST_TEST_MESSAGE( "Test from_account receiving no withdraw" );

      tx.operations.clear();
      tx.signatures.clear();

      op.to_account = "sam";
      op.percent = MUSE_1_PERCENT * 50;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.get_account( "alice" ).next_vesting_withdrawal, true );
      {
         const auto& alice = db.get_account( "alice" );
         const auto& bob = db.get_account( "bob" );
         const auto& sam = db.get_account( "sam" );

         BOOST_REQUIRE( alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate );
         BOOST_REQUIRE( alice.balance == old_alice_balance );
         BOOST_REQUIRE( bob.vesting_shares == old_bob_vesting + asset( ( vesting_withdraw_rate.amount * MUSE_1_PERCENT * 50 ) / MUSE_100_PERCENT, VESTS_SYMBOL ) );
         BOOST_REQUIRE( bob.balance == old_bob_balance );
         BOOST_REQUIRE( sam.vesting_shares == old_sam_vesting );
         BOOST_REQUIRE( sam.balance ==  old_sam_balance + asset( ( vesting_withdraw_rate.amount * MUSE_1_PERCENT * 50 ) / MUSE_100_PERCENT, VESTS_SYMBOL ) * db.get_dynamic_global_properties().get_vesting_share_price() );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( feed_publish_mean )
{
   try
   {
      ACTORS( (alice0)(alice1)(alice2)(alice3)(alice4)(alice5)(alice6) )

      BOOST_TEST_MESSAGE( "Setup" );

      generate_blocks( 30 / MUSE_BLOCK_INTERVAL );

      vector< string > accounts;
      accounts.push_back( "alice0" );
      accounts.push_back( "alice1" );
      accounts.push_back( "alice2" );
      accounts.push_back( "alice3" );
      accounts.push_back( "alice4" );
      accounts.push_back( "alice5" );
      accounts.push_back( "alice6" );

      vector< private_key_type > keys;
      keys.push_back( alice0_private_key );
      keys.push_back( alice1_private_key );
      keys.push_back( alice2_private_key );
      keys.push_back( alice3_private_key );
      keys.push_back( alice4_private_key );
      keys.push_back( alice5_private_key );
      keys.push_back( alice6_private_key );

      vector< feed_publish_operation > ops;
      vector< signed_transaction > txs;

      // Upgrade accounts to witnesses
      for( int i = 0; i < 7; i++ )
      {
         transfer( MUSE_INIT_MINER_NAME, accounts[i], 10000 );
         witness_create( accounts[i], keys[i], "foo.bar", keys[i].get_public_key(), 1000 );

         ops.push_back( feed_publish_operation() );
         ops[i].publisher = accounts[i];

         txs.push_back( signed_transaction() );
      }

      ops[0].exchange_rate = price( asset( 100000, MUSE_SYMBOL ), asset( 1000, MBD_SYMBOL ) );
      ops[1].exchange_rate = price( asset( 105000, MUSE_SYMBOL ), asset( 1000, MBD_SYMBOL ) );
      ops[2].exchange_rate = price( asset(  98000, MUSE_SYMBOL ), asset( 1000, MBD_SYMBOL ) );
      ops[3].exchange_rate = price( asset(  97000, MUSE_SYMBOL ), asset( 1000, MBD_SYMBOL ) );
      ops[4].exchange_rate = price( asset(  99000, MUSE_SYMBOL ), asset( 1000, MBD_SYMBOL ) );
      ops[5].exchange_rate = price( asset(  97500, MUSE_SYMBOL ), asset( 1000, MBD_SYMBOL ) );
      ops[6].exchange_rate = price( asset( 102000, MUSE_SYMBOL ), asset( 1000, MBD_SYMBOL ) );

      for( int i = 0; i < 7; i++ )
      {
         txs[i].set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
         txs[i].operations.push_back( ops[i] );
         txs[i].sign( keys[i], db.get_chain_id() );
         db.push_transaction( txs[i], 0 );
      }

      BOOST_TEST_MESSAGE( "Jump forward an hour" );

      generate_blocks( MUSE_BLOCKS_PER_HOUR ); // Jump forward 1 hour
      BOOST_TEST_MESSAGE( "Get feed history object" );
      feed_history_object feed_history = db.get_feed_history();
      BOOST_TEST_MESSAGE( "Check state" );
      BOOST_REQUIRE( feed_history.current_median_history == price( asset( 99000, MUSE_SYMBOL), asset( 1000, MBD_SYMBOL ) ) );
      BOOST_REQUIRE( feed_history.price_history[ 0 ] == price( asset( 99000, MUSE_SYMBOL), asset( 1000, MBD_SYMBOL ) ) );
      validate_database();

      for ( int i = 0; i < 23; i++ )
      {
         BOOST_TEST_MESSAGE( "Updating ops" );

         for( int j = 0; j < 7; j++ )
         {
            txs[j].operations.clear();
            txs[j].signatures.clear();
            ops[j].exchange_rate = price( ops[j].exchange_rate.base, asset( ops[j].exchange_rate.quote.amount + 10, MBD_SYMBOL ) );
            txs[j].set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
            txs[j].operations.push_back( ops[j] );
            txs[j].sign( keys[j], db.get_chain_id() );
            db.push_transaction( txs[j], 0 );
         }

         BOOST_TEST_MESSAGE( "Generate Blocks" );

         generate_blocks( MUSE_BLOCKS_PER_HOUR  ); // Jump forward 1 hour

         BOOST_TEST_MESSAGE( "Check feed_history" );

         feed_history = feed_history_id_type()( db );
         BOOST_REQUIRE( feed_history.current_median_history == feed_history.price_history[ ( i + 1 ) / 2 ] );
         BOOST_REQUIRE( feed_history.price_history[ i + 1 ] == ops[4].exchange_rate );
         validate_database();
      }
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( convert_delay )
{
   try
   {
      ACTORS( (alice) )

      set_price_feed( price( asset::from_string( "1.250 TESTS" ), asset::from_string( "1.000 TBD" ) ) );

      convert_operation op;
      comment_operation comment;
      vote_operation vote;
      signed_transaction tx;
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );

      comment.author = "alice";
      comment.title = "foo";
      comment.body = "bar";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      tx.operations.push_back( comment );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = MUSE_100_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.get_comment( "alice", "test" ).cashout_time, true );

      auto start_balance = asset( db.get_comment( "alice", "test" ).total_payout_value.amount / 2, MBD_SYMBOL );

      BOOST_TEST_MESSAGE( "Setup conversion to TESTS" );
      tx.operations.clear();
      tx.signatures.clear();
      op.owner = "alice";
      op.amount = asset( 2000, MBD_SYMBOL );
      op.requestid = 2;
      tx.operations.push_back( op );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Generating Blocks up to conversion block" );
      generate_blocks( db.head_block_time() + MUSE_CONVERSION_DELAY - fc::seconds( MUSE_BLOCK_INTERVAL / 2 ), true );

      BOOST_TEST_MESSAGE( "Verify conversion is not applied" );
      const auto& alice_2 = db.get_account( "alice" );
      const auto& convert_request_idx = db.get_index_type< convert_index >().indices().get< by_owner >();
      auto convert_request = convert_request_idx.find( std::make_tuple( "alice", 2 ) );

      BOOST_REQUIRE( convert_request != convert_request_idx.end() );
      BOOST_REQUIRE_EQUAL( alice_2.balance.amount.value, 0 );
      BOOST_REQUIRE_EQUAL( alice_2.mbd_balance.amount.value, ( start_balance - op.amount ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "Generate one more block" );
      generate_block();

      BOOST_TEST_MESSAGE( "Verify conversion applied" );
      const auto& alice_3 = db.get_account( "alice" );
      auto vop = get_last_operations( 1 )[0].get< fill_convert_request_operation >();

      convert_request = convert_request_idx.find( std::make_tuple( "alice", 2 ) );
      BOOST_REQUIRE( convert_request == convert_request_idx.end() );
      BOOST_REQUIRE_EQUAL( alice_3.balance.amount.value, 2500 );
      BOOST_REQUIRE_EQUAL( alice_3.mbd_balance.amount.value, ( start_balance - op.amount ).amount.value );
      BOOST_REQUIRE_EQUAL( vop.owner, "alice" );
      BOOST_REQUIRE_EQUAL( vop.requestid, 2 );
      BOOST_REQUIRE_EQUAL( vop.amount_in.amount.value, ASSET( "2.000 TBD" ).amount.value );
      BOOST_REQUIRE_EQUAL( vop.amount_out.amount.value, ASSET( "2.500 TESTS" ).amount.value );
      validate_database();
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( muse_inflation )
{
   try
   {
   /*
      BOOST_TEST_MESSAGE( "Testing MUSE Inflation until the vesting start block" );

      auto gpo = db.get_dynamic_global_properties();
      auto virtual_supply = gpo.virtual_supply;
      auto witness_name = db.get_scheduled_witness(1);
      auto old_witness_balance = db.get_account( witness_name ).balance;
      auto old_witness_shares = db.get_account( witness_name ).vesting_shares;

      auto new_rewards = std::max( MUSE_MIN_CONTENT_REWARD, asset( ( MUSE_CONTENT_APR * gpo.virtual_supply.amount ) / ( MUSE_BLOCKS_PER_YEAR * 100 ), MUSE_SYMBOL ) )
         + std::max( MUSE_MIN_CURATE_REWARD, asset( ( MUSE_CURATE_APR * gpo.virtual_supply.amount ) / ( MUSE_BLOCKS_PER_YEAR * 100 ), MUSE_SYMBOL ) );
      auto witness_pay = std::max( MUSE_MIN_PRODUCER_REWARD, asset( ( MUSE_PRODUCER_APR * gpo.virtual_supply.amount ) / ( MUSE_BLOCKS_PER_YEAR * 100 ), MUSE_SYMBOL ) );
      auto witness_pay_shares = asset( 0, VESTS_SYMBOL );
      auto new_vesting_muse = asset( 0, MUSE_SYMBOL );
      auto new_vesting_shares = gpo.total_vesting_shares;

      if ( db.get_account( witness_name ).vesting_shares.amount.value == 0 )
      {
         new_vesting_muse += witness_pay;
         new_vesting_shares += witness_pay * ( gpo.total_vesting_shares / gpo.total_vesting_fund_muse );
      }

      auto new_supply = gpo.current_supply + new_rewards + witness_pay + new_vesting_muse;
      new_rewards += gpo.total_reward_fund_muse;
      new_vesting_muse += gpo.total_vesting_fund_muse;

      generate_block();

      gpo = db.get_dynamic_global_properties();

      BOOST_REQUIRE_EQUAL( gpo.current_supply.amount.value, new_supply.amount.value );
      BOOST_REQUIRE_EQUAL( gpo.virtual_supply.amount.value, new_supply.amount.value );
      BOOST_REQUIRE_EQUAL( gpo.total_reward_fund_muse.amount.value, new_rewards.amount.value );
      BOOST_REQUIRE_EQUAL( gpo.total_vesting_fund_muse.amount.value, new_vesting_muse.amount.value );
      BOOST_REQUIRE_EQUAL( gpo.total_vesting_shares.amount.value, new_vesting_shares.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( witness_name ).balance.amount.value, ( old_witness_balance + witness_pay ).amount.value );

      validate_database();

      while( db.head_block_num() < MUSE_START_VESTING_BLOCK - 1)
      {
         virtual_supply = gpo.virtual_supply;
         witness_name = db.get_scheduled_witness(1);
         old_witness_balance = db.get_account( witness_name ).balance;
         old_witness_shares = db.get_account( witness_name ).vesting_shares;


         new_rewards = std::max( MUSE_MIN_CONTENT_REWARD, asset( ( MUSE_CONTENT_APR * gpo.virtual_supply.amount ) / ( MUSE_BLOCKS_PER_YEAR * 100 ), MUSE_SYMBOL ) )
            + std::max( MUSE_MIN_CURATE_REWARD, asset( ( MUSE_CURATE_APR * gpo.virtual_supply.amount ) / ( MUSE_BLOCKS_PER_YEAR * 100 ), MUSE_SYMBOL ) );
         witness_pay = std::max( MUSE_MIN_PRODUCER_REWARD, asset( ( MUSE_PRODUCER_APR * gpo.virtual_supply.amount ) / ( MUSE_BLOCKS_PER_YEAR * 100 ), MUSE_SYMBOL ) );
         new_vesting_muse = asset( 0, MUSE_SYMBOL );
         new_vesting_shares = gpo.total_vesting_shares;

         if ( db.get_account( witness_name ).vesting_shares.amount.value == 0 )
         {
            new_vesting_muse += witness_pay;
            witness_pay_shares = witness_pay * gpo.get_vesting_share_price();
            new_vesting_shares += witness_pay_shares;
            new_supply += witness_pay;
            witness_pay = asset( 0, MUSE_SYMBOL );
         }

         new_supply = gpo.current_supply + new_rewards + witness_pay + new_vesting_muse;
         new_rewards += gpo.total_reward_fund_muse;
         new_vesting_muse += gpo.total_vesting_fund_muse;

         generate_block();

         gpo = db.get_dynamic_global_properties();

         BOOST_REQUIRE_EQUAL( gpo.current_supply.amount.value, new_supply.amount.value );
         BOOST_REQUIRE_EQUAL( gpo.virtual_supply.amount.value, new_supply.amount.value );
         BOOST_REQUIRE_EQUAL( gpo.total_reward_fund_muse.amount.value, new_rewards.amount.value );
         BOOST_REQUIRE_EQUAL( gpo.total_vesting_fund_muse.amount.value, new_vesting_muse.amount.value );
         BOOST_REQUIRE_EQUAL( gpo.total_vesting_shares.amount.value, new_vesting_shares.amount.value );
         BOOST_REQUIRE_EQUAL( db.get_account( witness_name ).balance.amount.value, ( old_witness_balance + witness_pay ).amount.value );
         BOOST_REQUIRE_EQUAL( db.get_account( witness_name ).vesting_shares.amount.value, ( old_witness_shares + witness_pay_shares ).amount.value );

         validate_database();
      }

      BOOST_TEST_MESSAGE( "Testing up to the start block for miner voting" );

      while( db.head_block_num() < MUSE_START_MINER_VOTING_BLOCK - 1 )
      {
         virtual_supply = gpo.virtual_supply;
         witness_name = db.get_scheduled_witness(1);
         old_witness_balance = db.get_account( witness_name ).balance;

         new_rewards = std::max( MUSE_MIN_CONTENT_REWARD, asset( ( MUSE_CONTENT_APR * gpo.virtual_supply.amount ) / ( MUSE_BLOCKS_PER_YEAR * 100 ), MUSE_SYMBOL ) )
            + std::max( MUSE_MIN_CURATE_REWARD, asset( ( MUSE_CURATE_APR * gpo.virtual_supply.amount ) / ( MUSE_BLOCKS_PER_YEAR * 100 ), MUSE_SYMBOL ) );
         witness_pay = std::max( MUSE_MIN_PRODUCER_REWARD, asset( ( MUSE_PRODUCER_APR * gpo.virtual_supply.amount ) / ( MUSE_BLOCKS_PER_YEAR * 100 ), MUSE_SYMBOL ) );
         auto witness_pay_shares = asset( 0, VESTS_SYMBOL );
         new_vesting_muse = asset( ( witness_pay + new_rewards ).amount * 9, MUSE_SYMBOL );
         new_vesting_shares = gpo.total_vesting_shares;

         if ( db.get_account( witness_name ).vesting_shares.amount.value == 0 )
         {
            new_vesting_muse += witness_pay;
            witness_pay_shares = witness_pay * gpo.get_vesting_share_price();
            new_vesting_shares += witness_pay_shares;
            new_supply += witness_pay;
            witness_pay = asset( 0, MUSE_SYMBOL );
         }

         new_supply = gpo.current_supply + new_rewards + witness_pay + new_vesting_muse;
         new_rewards += gpo.total_reward_fund_muse;
         new_vesting_muse += gpo.total_vesting_fund_muse;

         generate_block();

         gpo = db.get_dynamic_global_properties();

         BOOST_REQUIRE_EQUAL( gpo.current_supply.amount.value, new_supply.amount.value );
         BOOST_REQUIRE_EQUAL( gpo.virtual_supply.amount.value, new_supply.amount.value );
         BOOST_REQUIRE_EQUAL( gpo.total_reward_fund_muse.amount.value, new_rewards.amount.value );
         BOOST_REQUIRE_EQUAL( gpo.total_vesting_fund_muse.amount.value, new_vesting_muse.amount.value );
         BOOST_REQUIRE_EQUAL( gpo.total_vesting_shares.amount.value, new_vesting_shares.amount.value );
         BOOST_REQUIRE_EQUAL( db.get_account( witness_name ).balance.amount.value, ( old_witness_balance + witness_pay ).amount.value );
         BOOST_REQUIRE_EQUAL( db.get_account( witness_name ).vesting_shares.amount.value, ( old_witness_shares + witness_pay_shares ).amount.value );

         validate_database();
      }

      for( int i = 0; i < MUSE_BLOCKS_PER_DAY; i++ )
      {
         virtual_supply = gpo.virtual_supply;
         witness_name = db.get_scheduled_witness(1);
         old_witness_balance = db.get_account( witness_name ).balance;

         new_rewards = std::max( MUSE_MIN_CONTENT_REWARD, asset( ( MUSE_CONTENT_APR * gpo.virtual_supply.amount ) / ( MUSE_BLOCKS_PER_YEAR * 100 ), MUSE_SYMBOL ) )
            + std::max( MUSE_MIN_CURATE_REWARD, asset( ( MUSE_CURATE_APR * gpo.virtual_supply.amount ) / ( MUSE_BLOCKS_PER_YEAR * 100 ), MUSE_SYMBOL ) );
         witness_pay = std::max( MUSE_MIN_PRODUCER_REWARD, asset( ( MUSE_PRODUCER_APR * gpo.virtual_supply.amount ) / ( MUSE_BLOCKS_PER_YEAR * 100 ), MUSE_SYMBOL ) );
         witness_pay_shares = witness_pay * gpo.get_vesting_share_price();
         new_vesting_muse = asset( ( witness_pay + new_rewards ).amount * 9, MUSE_SYMBOL ) + witness_pay;
         new_vesting_shares = gpo.total_vesting_shares + witness_pay_shares;
         new_supply = gpo.current_supply + new_rewards + new_vesting_muse;
         new_rewards += gpo.total_reward_fund_muse;
         new_vesting_muse += gpo.total_vesting_fund_muse;

         generate_block();

         gpo = db.get_dynamic_global_properties();

         BOOST_REQUIRE_EQUAL( gpo.current_supply.amount.value, new_supply.amount.value );
         BOOST_REQUIRE_EQUAL( gpo.virtual_supply.amount.value, new_supply.amount.value );
         BOOST_REQUIRE_EQUAL( gpo.total_reward_fund_muse.amount.value, new_rewards.amount.value );
         BOOST_REQUIRE_EQUAL( gpo.total_vesting_fund_muse.amount.value, new_vesting_muse.amount.value );
         BOOST_REQUIRE_EQUAL( gpo.total_vesting_shares.amount.value, new_vesting_shares.amount.value );
         BOOST_REQUIRE_EQUAL( db.get_account( witness_name ).vesting_shares.amount.value, ( old_witness_shares + witness_pay_shares ).amount.value );

         validate_database();
      }
/*
      virtual_supply = gpo.virtual_supply;
      vesting_shares = gpo.total_vesting_shares;
      vesting_muse = gpo.total_vesting_fund_muse;
      reward_muse = gpo.total_reward_fund_muse;

      witness_name = db.get_scheduled_witness(1);
      old_witness_shares = db.get_account( witness_name ).vesting_shares;

      generate_block();

      gpo = db.get_dynamic_global_properties();

      BOOST_REQUIRE_EQUAL( gpo.total_vesting_fund_muse.amount.value,
         ( vesting_muse.amount.value
            + ( ( ( uint128_t( virtual_supply.amount.value ) / 10 ) / MUSE_BLOCKS_PER_YEAR ) * 9 )
            + ( uint128_t( virtual_supply.amount.value ) / 100 / MUSE_BLOCKS_PER_YEAR ) ).to_uint64() );
      BOOST_REQUIRE_EQUAL( gpo.total_reward_fund_muse.amount.value,
         reward_muse.amount.value + virtual_supply.amount.value / 10 / MUSE_BLOCKS_PER_YEAR + virtual_supply.amount.value / 10 / MUSE_BLOCKS_PER_DAY );
      BOOST_REQUIRE_EQUAL( db.get_account( witness_name ).vesting_shares.amount.value,
         old_witness_shares.amount.value + ( asset( ( ( virtual_supply.amount.value / MUSE_BLOCKS_PER_YEAR ) * MUSE_1_PERCENT ) / MUSE_100_PERCENT, MUSE_SYMBOL ) * ( vesting_shares / vesting_muse ) ).amount.value );
      validate_database();
      */
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( mbd_interest )
{
   try
   {
      ACTORS( (alice)(bob) )

      set_price_feed( price( asset::from_string( "1.000 TESTS" ), asset::from_string( "1.000 TBD" ) ) );

      BOOST_TEST_MESSAGE( "Testing interest over smallest interest period" );

      convert_operation op;
      comment_operation comment;
      vote_operation vote;
      signed_transaction tx;
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );

      comment.author = "alice";
      comment.title = "foo";
      comment.body = "bar";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      tx.operations.push_back( comment );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = MUSE_100_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.get_comment( "alice", "test" ).cashout_time, true );

      auto start_time = db.get_account( "alice" ).mbd_seconds_last_update;
      auto alice_mbd = db.get_account( "alice" ).mbd_balance;

      generate_blocks( db.head_block_time() + fc::seconds( MUSE_SBD_INTEREST_COMPOUND_INTERVAL_SEC ), true );

      transfer_operation transfer;
      transfer.to = "bob";
      transfer.from = "alice";
      transfer.amount = ASSET( "1.000 TBD" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( transfer );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      auto gpo = db.get_dynamic_global_properties();
      auto interest_op = get_last_operations( 1 )[0].get< interest_operation >();

      BOOST_REQUIRE( gpo.mbd_interest_rate > 0 );
      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).mbd_balance.amount.value, alice_mbd.amount.value - ASSET( "1.000 TBD" ).amount.value + ( ( ( ( uint128_t( alice_mbd.amount.value ) * ( db.head_block_time() - start_time ).to_seconds() ) / MUSE_SECONDS_PER_YEAR ) * gpo.mbd_interest_rate ) / MUSE_100_PERCENT ).to_uint64() );
      BOOST_REQUIRE_EQUAL( interest_op.owner, "alice" );
      BOOST_REQUIRE_EQUAL( interest_op.interest.amount.value, db.get_account( "alice" ).mbd_balance.amount.value - ( alice_mbd.amount.value - ASSET( "1.000 TBD" ).amount.value ) );
      validate_database();

      BOOST_TEST_MESSAGE( "Testing interest under interest period" );

      start_time = db.get_account( "alice" ).mbd_seconds_last_update;
      alice_mbd = db.get_account( "alice" ).mbd_balance;

      generate_blocks( db.head_block_time() + fc::seconds( MUSE_SBD_INTEREST_COMPOUND_INTERVAL_SEC / 2 ), true );

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( transfer );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).mbd_balance.amount.value, alice_mbd.amount.value - ASSET( "1.000 TBD" ).amount.value );
      validate_database();

      auto alice_coindays = uint128_t( alice_mbd.amount.value ) * ( db.head_block_time() - start_time ).to_seconds();
      alice_mbd = db.get_account( "alice" ).mbd_balance;
      start_time = db.get_account( "alice" ).mbd_seconds_last_update;

      BOOST_TEST_MESSAGE( "Testing longer interest period" );

      generate_blocks( db.head_block_time() + fc::seconds( ( MUSE_SBD_INTEREST_COMPOUND_INTERVAL_SEC * 7 ) / 3 ), true );

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( transfer );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).mbd_balance.amount.value, alice_mbd.amount.value - ASSET( "1.000 TBD" ).amount.value + ( ( ( ( uint128_t( alice_mbd.amount.value ) * ( db.head_block_time() - start_time ).to_seconds() + alice_coindays ) / MUSE_SECONDS_PER_YEAR ) * gpo.mbd_interest_rate ) / MUSE_100_PERCENT ).to_uint64() );
      validate_database();
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( liquidity_rewards )
{
   using std::abs;

   try
   {
      db.liquidity_rewards_enabled = false;

      ACTORS( (alice)(bob)(sam)(dave) )

      BOOST_TEST_MESSAGE( "Rewarding Bob with TESTS" );

      auto exchange_rate = price( ASSET( "1.250 TESTS" ), ASSET( "1.000 TBD" ) );
      set_price_feed( exchange_rate );

      signed_transaction tx;
      comment_operation comment;
      comment.author = "alice";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      comment.title = "foo";
      comment.body = "bar";
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( comment );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      vote_operation vote;
      vote.voter = "alice";
      vote.weight = MUSE_100_PERCENT;
      vote.author = "alice";
      vote.permlink = "test";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.get_comment( "alice", "test" ).cashout_time, true );
      asset alice_mbd = db.get_account( "alice" ).mbd_balance;

      fund( "alice", alice_mbd.amount );
      fund( "bob", alice_mbd.amount );
      fund( "sam", alice_mbd.amount );
      fund( "dave", alice_mbd.amount );

      int64_t alice_mbd_volume = 0;
      int64_t alice_muse_volume = 0;
      time_point_sec alice_reward_last_update = fc::time_point_sec::min();
      int64_t bob_mbd_volume = 0;
      int64_t bob_muse_volume = 0;
      time_point_sec bob_reward_last_update = fc::time_point_sec::min();
      int64_t sam_mbd_volume = 0;
      int64_t sam_muse_volume = 0;
      time_point_sec sam_reward_last_update = fc::time_point_sec::min();
      int64_t dave_mbd_volume = 0;
      int64_t dave_muse_volume = 0;
      time_point_sec dave_reward_last_update = fc::time_point_sec::min();

      BOOST_TEST_MESSAGE( "Creating Limit Order for MUSE that will stay on the books for 30 minutes exactly." );

      limit_order_create_operation op;
      op.owner = "alice";
      op.amount_to_sell = asset( alice_mbd.amount.value / 20, MBD_SYMBOL ) ;
      op.min_to_receive = op.amount_to_sell * exchange_rate;
      op.orderid = 1;

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Waiting 10 minutes" );

      generate_blocks( db.head_block_time() + MUSE_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10, true );

      BOOST_TEST_MESSAGE( "Creating Limit Order for SBD that will be filled immediately." );

      op.owner = "bob";
      op.min_to_receive = op.amount_to_sell;
      op.amount_to_sell = op.min_to_receive * exchange_rate;
      op.fill_or_kill = false;
      op.orderid = 2;

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      alice_muse_volume += ( asset( alice_mbd.amount / 20, MBD_SYMBOL ) * exchange_rate ).amount.value;
      alice_reward_last_update = db.head_block_time();
      bob_muse_volume -= ( asset( alice_mbd.amount / 20, MBD_SYMBOL ) * exchange_rate ).amount.value;
      bob_reward_last_update = db.head_block_time();

      auto ops = get_last_operations( 1 );
      const auto& liquidity_idx = db.get_index_type< liquidity_reward_index >().indices().get< by_owner >();
      const auto& limit_order_idx = db.get_index_type< limit_order_index >().indices().get< by_account >();

      auto reward = liquidity_idx.find( db.get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "alice" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, alice_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, alice_muse_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "bob" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, bob_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, bob_muse_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      auto fill_order_op = ops[0].get< fill_order_operation >();

      BOOST_CHECK_EQUAL( fill_order_op.open_owner, "alice" );
      BOOST_CHECK_EQUAL( fill_order_op.open_orderid, 1 );
      BOOST_CHECK_EQUAL( fill_order_op.open_pays.amount.value, asset( alice_mbd.amount.value / 20, MBD_SYMBOL ).amount.value );
      BOOST_CHECK_EQUAL( fill_order_op.current_owner, "bob" );
      BOOST_CHECK_EQUAL( fill_order_op.current_orderid, 2 );
      BOOST_CHECK_EQUAL( fill_order_op.current_pays.amount.value, ( asset( alice_mbd.amount.value / 20, MBD_SYMBOL ) * exchange_rate ).amount.value );

      BOOST_CHECK( limit_order_idx.find( std::make_tuple( "alice", 1 ) ) == limit_order_idx.end() );
      BOOST_CHECK( limit_order_idx.find( std::make_tuple( "bob", 2 ) ) == limit_order_idx.end() );

      BOOST_TEST_MESSAGE( "Creating Limit Order for SBD that will stay on the books for 60 minutes." );

      op.owner = "sam";
      op.amount_to_sell = asset( ( alice_mbd.amount.value / 20 ), MUSE_SYMBOL );
      op.min_to_receive = asset( ( alice_mbd.amount.value / 20 ), MBD_SYMBOL );
      op.orderid = 3;

      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Waiting 10 minutes" );

      generate_blocks( db.head_block_time() + MUSE_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10, true );

      BOOST_TEST_MESSAGE( "Creating Limit Order for SBD that will stay on the books for 30 minutes." );

      op.owner = "bob";
      op.orderid = 4;
      op.amount_to_sell = asset( ( alice_mbd.amount.value / 10 ) * 3 - alice_mbd.amount.value / 20, MUSE_SYMBOL );
      op.min_to_receive = asset( ( alice_mbd.amount.value / 10 ) * 3 - alice_mbd.amount.value / 20, MBD_SYMBOL );

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Waiting 30 minutes" );

      generate_blocks( db.head_block_time() + MUSE_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10, true );

      BOOST_TEST_MESSAGE( "Filling both limit orders." );

      op.owner = "alice";
      op.orderid = 5;
      op.amount_to_sell = asset( ( alice_mbd.amount.value / 10 ) * 3, MBD_SYMBOL );
      op.min_to_receive = asset( ( alice_mbd.amount.value / 10 ) * 3, MUSE_SYMBOL );

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      alice_mbd_volume -= ( alice_mbd.amount.value / 10 ) * 3;
      alice_reward_last_update = db.head_block_time();
      sam_mbd_volume += alice_mbd.amount.value / 20;
      sam_reward_last_update = db.head_block_time();
      bob_mbd_volume += ( alice_mbd.amount.value / 10 ) * 3 - ( alice_mbd.amount.value / 20 );
      bob_reward_last_update = db.head_block_time();
      ops = get_last_operations( 4 );

      fill_order_op = ops[1].get< fill_order_operation >();
      BOOST_CHECK_EQUAL( fill_order_op.open_owner, "bob" );
      BOOST_CHECK_EQUAL( fill_order_op.open_orderid, 4 );
      BOOST_CHECK_EQUAL( fill_order_op.open_pays.amount.value, asset( ( alice_mbd.amount.value / 10 ) * 3 - alice_mbd.amount.value / 20, MUSE_SYMBOL ).amount.value );
      BOOST_CHECK_EQUAL( fill_order_op.current_owner, "alice" );
      BOOST_CHECK_EQUAL( fill_order_op.current_orderid, 5 );
      BOOST_CHECK_EQUAL( fill_order_op.current_pays.amount.value, asset( ( alice_mbd.amount.value / 10 ) * 3 - alice_mbd.amount.value / 20, MBD_SYMBOL ).amount.value );

      fill_order_op = ops[3].get< fill_order_operation >();
      BOOST_CHECK_EQUAL( fill_order_op.open_owner, "sam" );
      BOOST_CHECK_EQUAL( fill_order_op.open_orderid, 3 );
      BOOST_CHECK_EQUAL( fill_order_op.open_pays.amount.value, asset( alice_mbd.amount.value / 20, MUSE_SYMBOL ).amount.value );
      BOOST_CHECK_EQUAL( fill_order_op.current_owner, "alice" );
      BOOST_CHECK_EQUAL( fill_order_op.current_orderid, 5 );
      BOOST_CHECK_EQUAL( fill_order_op.current_pays.amount.value, asset( alice_mbd.amount.value / 20, MBD_SYMBOL ).amount.value );

      reward = liquidity_idx.find( db.get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "alice" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, alice_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, alice_muse_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "bob" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, bob_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, bob_muse_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "sam" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "sam" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, sam_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, sam_muse_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      BOOST_TEST_MESSAGE( "Testing a partial fill before minimum time and full fill after minimum time" );

      op.orderid = 6;
      op.amount_to_sell = asset( alice_mbd.amount.value / 20 * 2, MBD_SYMBOL );
      op.min_to_receive = asset( alice_mbd.amount.value / 20 * 2, MUSE_SYMBOL );

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.head_block_time() + fc::seconds( MUSE_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10.to_seconds() / 2 ), true );

      op.owner = "bob";
      op.orderid = 7;
      op.amount_to_sell = asset( alice_mbd.amount.value / 20, MUSE_SYMBOL );
      op.min_to_receive = asset( alice_mbd.amount.value / 20, MBD_SYMBOL );

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.head_block_time() + fc::seconds( MUSE_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10.to_seconds() / 2 ), true );

      ops = get_last_operations( 1 );
      fill_order_op = ops[0].get< fill_order_operation >();

      BOOST_CHECK_EQUAL( fill_order_op.open_owner, "alice" );
      BOOST_CHECK_EQUAL( fill_order_op.open_orderid, 6 );
      BOOST_CHECK_EQUAL( fill_order_op.open_pays.amount.value, asset( alice_mbd.amount.value / 20, MBD_SYMBOL ).amount.value );
      BOOST_CHECK_EQUAL( fill_order_op.current_owner, "bob" );
      BOOST_CHECK_EQUAL( fill_order_op.current_orderid, 7 );
      BOOST_CHECK_EQUAL( fill_order_op.current_pays.amount.value, asset( alice_mbd.amount.value / 20, MUSE_SYMBOL ).amount.value );

      reward = liquidity_idx.find( db.get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "alice" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, alice_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, alice_muse_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "bob" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, bob_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, bob_muse_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "sam" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "sam" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, sam_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, sam_muse_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      generate_blocks( db.head_block_time() + MUSE_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10, true );

      op.owner = "sam";
      op.orderid = 8;

      tx.signatures.clear();
      tx.operations.clear();
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      alice_muse_volume += alice_mbd.amount.value / 20;
      alice_reward_last_update = db.head_block_time();
      sam_muse_volume -= alice_mbd.amount.value / 20;
      sam_reward_last_update = db.head_block_time();

      ops = get_last_operations( 2 );
      fill_order_op = ops[1].get< fill_order_operation >();

      BOOST_CHECK_EQUAL( fill_order_op.open_owner, "alice" );
      BOOST_CHECK_EQUAL( fill_order_op.open_orderid, 6 );
      BOOST_CHECK_EQUAL( fill_order_op.open_pays.amount.value, asset( alice_mbd.amount.value / 20, MBD_SYMBOL ).amount.value );
      BOOST_CHECK_EQUAL( fill_order_op.current_owner, "sam" );
      BOOST_CHECK_EQUAL( fill_order_op.current_orderid, 8 );
      BOOST_CHECK_EQUAL( fill_order_op.current_pays.amount.value, asset( alice_mbd.amount.value / 20, MUSE_SYMBOL ).amount.value );

      reward = liquidity_idx.find( db.get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "alice" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, alice_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, alice_muse_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "bob" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, bob_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, bob_muse_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "sam" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "sam" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, sam_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, sam_muse_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      BOOST_TEST_MESSAGE( "Trading to give Alice and Bob positive volumes to receive rewards" );

      transfer_operation transfer;
      transfer.to = "dave";
      transfer.from = "alice";
      transfer.amount = asset( alice_mbd.amount / 2, MBD_SYMBOL );

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( transfer );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      op.owner = "alice";
      op.amount_to_sell = asset( 8 * ( alice_mbd.amount.value / 20 ), MUSE_SYMBOL );
      op.min_to_receive = asset( op.amount_to_sell.amount, MBD_SYMBOL );
      op.orderid = 9;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.head_block_time() + MUSE_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10, true );

      op.owner = "dave";
      op.amount_to_sell = asset( 7 * ( alice_mbd.amount.value / 20 ), MBD_SYMBOL );;
      op.min_to_receive = asset( op.amount_to_sell.amount, MUSE_SYMBOL );
      op.orderid = 10;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      alice_mbd_volume += op.amount_to_sell.amount.value;
      alice_reward_last_update = db.head_block_time();
      dave_mbd_volume -= op.amount_to_sell.amount.value;
      dave_reward_last_update = db.head_block_time();

      ops = get_last_operations( 1 );
      fill_order_op = ops[0].get< fill_order_operation >();

      BOOST_CHECK_EQUAL( fill_order_op.open_owner, "alice" );
      BOOST_CHECK_EQUAL( fill_order_op.open_orderid, 9 );
      BOOST_CHECK_EQUAL( fill_order_op.open_pays.amount.value, 7 * ( alice_mbd.amount.value / 20 ) );
      BOOST_CHECK_EQUAL( fill_order_op.current_owner, "dave" );
      BOOST_CHECK_EQUAL( fill_order_op.current_orderid, 10 );
      BOOST_CHECK_EQUAL( fill_order_op.current_pays.amount.value, 7 * ( alice_mbd.amount.value / 20 ) );

      reward = liquidity_idx.find( db.get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "alice" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, alice_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, alice_muse_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "bob" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, bob_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, bob_muse_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "sam" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "sam" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, sam_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, sam_muse_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "dave" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "dave" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, dave_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, dave_muse_volume );
      BOOST_CHECK( reward->last_update == dave_reward_last_update );*/

      op.owner = "bob";
      op.amount_to_sell.amount = alice_mbd.amount / 20;
      op.min_to_receive.amount = op.amount_to_sell.amount;
      op.orderid = 11;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      alice_mbd_volume += op.amount_to_sell.amount.value;
      alice_reward_last_update = db.head_block_time();
      bob_mbd_volume -= op.amount_to_sell.amount.value;
      bob_reward_last_update = db.head_block_time();

      ops = get_last_operations( 1 );
      fill_order_op = ops[0].get< fill_order_operation >();

      BOOST_CHECK_EQUAL( fill_order_op.open_owner, "alice" );
      BOOST_CHECK_EQUAL( fill_order_op.open_orderid, 9 );
      BOOST_CHECK_EQUAL( fill_order_op.open_pays.amount.value, alice_mbd.amount.value / 20 );
      BOOST_CHECK_EQUAL( fill_order_op.current_owner, "bob" );
      BOOST_CHECK_EQUAL( fill_order_op.current_orderid, 11 );
      BOOST_CHECK_EQUAL( fill_order_op.current_pays.amount.value, alice_mbd.amount.value / 20 );

      reward = liquidity_idx.find( db.get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "alice" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, alice_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, alice_muse_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "bob" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, bob_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, bob_muse_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "sam" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "sam" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, sam_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, sam_muse_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "dave" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "dave" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, dave_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, dave_muse_volume );
      BOOST_CHECK( reward->last_update == dave_reward_last_update );*/

      transfer.to = "bob";
      transfer.from = "alice";
      transfer.amount = asset( alice_mbd.amount / 5, MBD_SYMBOL );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( transfer );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 12;
      op.amount_to_sell = asset( 3 * ( alice_mbd.amount / 40 ), MBD_SYMBOL );
      op.min_to_receive = asset( op.amount_to_sell.amount, MUSE_SYMBOL );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.head_block_time() + MUSE_MIN_LIQUIDITY_REWARD_PERIOD_SEC_HF10, true );

      op.owner = "dave";
      op.orderid = 13;
      op.amount_to_sell = op.min_to_receive;
      op.min_to_receive.symbol = MBD_SYMBOL;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      bob_muse_volume += op.amount_to_sell.amount.value;
      bob_reward_last_update = db.head_block_time();
      dave_muse_volume -= op.amount_to_sell.amount.value;
      dave_reward_last_update = db.head_block_time();

      ops = get_last_operations( 1 );
      fill_order_op = ops[0].get< fill_order_operation >();

      BOOST_CHECK_EQUAL( fill_order_op.open_owner, "bob" );
      BOOST_CHECK_EQUAL( fill_order_op.open_orderid, 12 );
      BOOST_CHECK_EQUAL( fill_order_op.open_pays.amount.value, 3 * ( alice_mbd.amount.value / 40 ) );
      BOOST_CHECK_EQUAL( fill_order_op.current_owner, "dave" );
      BOOST_CHECK_EQUAL( fill_order_op.current_orderid, 13 );
      BOOST_CHECK_EQUAL( fill_order_op.current_pays.amount.value, 3 * ( alice_mbd.amount.value / 40 ) );

      reward = liquidity_idx.find( db.get_account( "alice" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "alice" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, alice_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, alice_muse_volume );
      BOOST_CHECK( reward->last_update == alice_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "bob" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "bob" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, bob_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, bob_muse_volume );
      BOOST_CHECK( reward->last_update == bob_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "sam" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "sam" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, sam_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, sam_muse_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      reward = liquidity_idx.find( db.get_account( "dave" ).id );
      BOOST_REQUIRE( reward == liquidity_idx.end() );
      /*BOOST_CHECK_EQUAL( reward->owner, db.get_account( "dave" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, dave_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, dave_muse_volume );
      BOOST_CHECK( reward->last_update == dave_reward_last_update );*/

      auto dave_last_order_time = db.head_block_time();

      auto alice_balance = db.get_account( "alice" ).balance;
      auto bob_balance = db.get_account( "bob" ).balance;
      auto sam_balance = db.get_account( "sam" ).balance;
      auto dave_balance = db.get_account( "dave" ).balance;

      BOOST_TEST_MESSAGE( "Generating Blocks to trigger liquidity rewards" );

      db.liquidity_rewards_enabled = true;
      generate_blocks( MUSE_LIQUIDITY_REWARD_BLOCKS - ( db.head_block_num() % MUSE_LIQUIDITY_REWARD_BLOCKS ) - 1 );

      BOOST_REQUIRE( db.head_block_num() % MUSE_LIQUIDITY_REWARD_BLOCKS == MUSE_LIQUIDITY_REWARD_BLOCKS - 1 );
      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).balance.amount.value, alice_balance.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "bob" ).balance.amount.value, bob_balance.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "sam" ).balance.amount.value, sam_balance.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "dave" ).balance.amount.value, dave_balance.amount.value );

      generate_block();

      //alice_balance += MUSE_MIN_LIQUIDITY_REWARD;

      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).balance.amount.value, alice_balance.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "bob" ).balance.amount.value, bob_balance.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "sam" ).balance.amount.value, sam_balance.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "dave" ).balance.amount.value, dave_balance.amount.value );

      ops = get_last_operations( 1 );

      MUSE_REQUIRE_THROW( ops[0].get< liquidity_reward_operation>(), fc::assert_exception );
      //BOOST_REQUIRE_EQUAL( ops[0].get< liquidity_reward_operation>().payout.amount.value, MUSE_MIN_LIQUIDITY_REWARD.amount.value );

      generate_blocks( MUSE_LIQUIDITY_REWARD_BLOCKS );

      //bob_balance += MUSE_MIN_LIQUIDITY_REWARD;

      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).balance.amount.value, alice_balance.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "bob" ).balance.amount.value, bob_balance.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "sam" ).balance.amount.value, sam_balance.amount.value );
      BOOST_REQUIRE_EQUAL( db.get_account( "dave" ).balance.amount.value, dave_balance.amount.value );

      ops = get_last_operations( 1 );

      MUSE_REQUIRE_THROW( ops[0].get< liquidity_reward_operation>(), fc::assert_exception );
      //BOOST_REQUIRE_EQUAL( ops[0].get< liquidity_reward_operation>().payout.amount.value, MUSE_MIN_LIQUIDITY_REWARD.amount.value );

      alice_muse_volume = 0;
      alice_mbd_volume = 0;
      bob_muse_volume = 0;
      bob_mbd_volume = 0;

      BOOST_TEST_MESSAGE( "Testing liquidity timeout" );

      generate_blocks( sam_reward_last_update + MUSE_LIQUIDITY_TIMEOUT_SEC - fc::seconds( MUSE_BLOCK_INTERVAL / 2 ) - MUSE_MIN_LIQUIDITY_REWARD_PERIOD_SEC , true );

      op.owner = "sam";
      op.orderid = 14;
      op.amount_to_sell = ASSET( "1.000 TESTS" );
      op.min_to_receive = ASSET( "1.000 TBD" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.head_block_time() + ( MUSE_BLOCK_INTERVAL / 2 ) + MUSE_LIQUIDITY_TIMEOUT_SEC, true );

      reward = liquidity_idx.find( db.get_account( "sam" ).id );
      /*BOOST_REQUIRE( reward == liquidity_idx.end() );
      BOOST_CHECK_EQUAL( reward->owner, db.get_account( "sam" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, sam_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, sam_muse_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/

      generate_block();

      op.owner = "alice";
      op.orderid = 15;
      op.amount_to_sell.symbol = MBD_SYMBOL;
      op.min_to_receive.symbol = MUSE_SYMBOL;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      sam_mbd_volume = ASSET( "1.000 TBD" ).amount.value;
      sam_muse_volume = 0;
      sam_reward_last_update = db.head_block_time();

      reward = liquidity_idx.find( db.get_account( "sam" ).id );
      /*BOOST_REQUIRE( reward == liquidity_idx.end() );
      BOOST_CHECK_EQUAL( reward->owner, db.get_account( "sam" ).id );
      BOOST_CHECK_EQUAL( reward->mbd_volume, sam_mbd_volume );
      BOOST_CHECK_EQUAL( reward->muse_volume, sam_muse_volume );
      BOOST_CHECK( reward->last_update == sam_reward_last_update );*/
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( post_rate_limit )
{
   try
   {
      ACTORS( (alice) )

      fund( "alice", 10000 );
      vest( "alice", 10000 );

      comment_operation op;
      op.author = "alice";
      op.permlink = "test1";
      op.parent_author = "";
      op.parent_permlink = "test";
      op.body = "test";

      signed_transaction tx;

      tx.operations.push_back( op );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      uint64_t alice_post_bandwidth = MUSE_100_PERCENT;

      BOOST_REQUIRE( alice.post_bandwidth == alice_post_bandwidth );
      BOOST_REQUIRE( db.get_comment( "alice", "test1" ).reward_weight == MUSE_100_PERCENT );

      tx.operations.clear();
      tx.signatures.clear();

      generate_blocks( db.head_block_time() + MUSE_MIN_ROOT_COMMENT_INTERVAL + fc::seconds( MUSE_BLOCK_INTERVAL ), true );

      op.permlink = "test2";

      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      alice_post_bandwidth = MUSE_100_PERCENT + ( alice_post_bandwidth * ( MUSE_POST_AVERAGE_WINDOW - MUSE_MIN_ROOT_COMMENT_INTERVAL.to_seconds() - MUSE_BLOCK_INTERVAL ) / MUSE_POST_AVERAGE_WINDOW );

      BOOST_REQUIRE( db.get_account( "alice" ).post_bandwidth == alice_post_bandwidth );
      BOOST_REQUIRE( db.get_comment( "alice", "test2" ).reward_weight == MUSE_100_PERCENT );

      generate_blocks( db.head_block_time() + MUSE_MIN_ROOT_COMMENT_INTERVAL + fc::seconds( MUSE_BLOCK_INTERVAL ), true );

      tx.operations.clear();
      tx.signatures.clear();

      op.permlink = "test3";

      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      alice_post_bandwidth = MUSE_100_PERCENT + ( alice_post_bandwidth * ( MUSE_POST_AVERAGE_WINDOW - MUSE_MIN_ROOT_COMMENT_INTERVAL.to_seconds() - MUSE_BLOCK_INTERVAL ) / MUSE_POST_AVERAGE_WINDOW );

      BOOST_REQUIRE( db.get_account( "alice" ).post_bandwidth == alice_post_bandwidth );
      BOOST_REQUIRE( db.get_comment( "alice", "test3" ).reward_weight == MUSE_100_PERCENT );

      generate_blocks( db.head_block_time() + MUSE_MIN_ROOT_COMMENT_INTERVAL + fc::seconds( MUSE_BLOCK_INTERVAL ), true );

      tx.operations.clear();
      tx.signatures.clear();

      op.permlink = "test4";

      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      alice_post_bandwidth = MUSE_100_PERCENT + ( alice_post_bandwidth * ( MUSE_POST_AVERAGE_WINDOW - MUSE_MIN_ROOT_COMMENT_INTERVAL.to_seconds() - MUSE_BLOCK_INTERVAL ) / MUSE_POST_AVERAGE_WINDOW );

      BOOST_REQUIRE( db.get_account( "alice" ).post_bandwidth == alice_post_bandwidth );
      BOOST_REQUIRE( db.get_comment( "alice", "test4" ).reward_weight == MUSE_100_PERCENT );

      generate_blocks( db.head_block_time() + MUSE_MIN_ROOT_COMMENT_INTERVAL + fc::seconds( MUSE_BLOCK_INTERVAL ), true );

      tx.operations.clear();
      tx.signatures.clear();

      op.permlink = "test5";

      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      alice_post_bandwidth = MUSE_100_PERCENT + ( alice_post_bandwidth * ( MUSE_POST_AVERAGE_WINDOW - MUSE_MIN_ROOT_COMMENT_INTERVAL.to_seconds() - MUSE_BLOCK_INTERVAL ) / MUSE_POST_AVERAGE_WINDOW );
      auto reward_weight = ( MUSE_POST_WEIGHT_CONSTANT * MUSE_100_PERCENT ) / ( alice_post_bandwidth * alice_post_bandwidth );

      BOOST_REQUIRE( db.get_account( "alice" ).post_bandwidth == alice_post_bandwidth );
      BOOST_REQUIRE( db.get_comment( "alice", "test5" ).reward_weight == reward_weight );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( comment_freeze )
{
   try
   {
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );
      fund( "bob", 10000 );
      fund( "sam", 10000 );
      fund( "dave", 10000 );

      vest( "alice", 10000 );
      vest( "bob", 10000 );
      vest( "sam", 10000 );
      vest( "dave", 10000 );

      auto exchange_rate = price( ASSET( "1.250 TESTS" ), ASSET( "1.000 TBD" ) );
      set_price_feed( exchange_rate );

      signed_transaction tx;

      comment_operation comment;
      comment.author = "alice";
      comment.parent_author = "";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      comment.body = "test";

      tx.operations.push_back( comment );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      comment.body = "test2";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( comment );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      vote_operation vote;
      vote.weight = MUSE_100_PERCENT;
      vote.voter = "bob";
      vote.author = "alice";
      vote.permlink = "test";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_REQUIRE( db.get_comment( "alice", "test" ).last_payout == fc::time_point_sec::min() );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).cashout_time != fc::time_point_sec::min() );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).cashout_time != fc::time_point_sec::maximum() );

      generate_blocks( db.get_comment( "alice", "test" ).cashout_time, true );

      BOOST_REQUIRE( db.get_comment( "alice", "test" ).last_payout == db.head_block_time() );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).cashout_time == db.head_block_time() + MUSE_SECOND_CASHOUT_WINDOW );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), fc::assert_exception );

      vote.voter = "sam";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      comment.body = "test3";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( comment );
      tx.sign( alice_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), fc::assert_exception );

      generate_blocks( db.get_comment( "alice", "test" ).cashout_time, true );

      BOOST_REQUIRE( db.get_comment( "alice", "test" ).last_payout == db.head_block_time() );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).cashout_time == fc::time_point_sec::maximum() );

      vote.voter = "sam";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_REQUIRE( db.get_comment( "alice", "test" ).cashout_time == fc::time_point_sec::maximum() );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).net_rshares.value == 0 );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).abs_rshares.value == 0 );

      vote.voter = "bob";
      vote.weight = MUSE_100_PERCENT * -1;

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_REQUIRE( db.get_comment( "alice", "test" ).cashout_time == fc::time_point_sec::maximum() );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).net_rshares.value == 0 );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).abs_rshares.value == 0 );

      vote.voter = "dave";
      vote.weight = 0;

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( dave_private_key, db.get_chain_id() );

      db.push_transaction( tx, 0 );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).cashout_time == fc::time_point_sec::maximum() );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).net_rshares.value == 0 );
      BOOST_REQUIRE( db.get_comment( "alice", "test" ).abs_rshares.value == 0 );

      comment.body = "test4";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( comment );
      tx.sign( alice_private_key, db.get_chain_id() );
      MUSE_REQUIRE_THROW( db.push_transaction( tx, 0 ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_SUITE_END()
#endif
