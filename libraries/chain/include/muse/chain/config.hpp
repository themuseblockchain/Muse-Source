/*
 * Copyright (c) 2016 Peer Tracks, Inc., and contributors.
 */
#pragma once

#define MUSE_BLOCKCHAIN_VERSION              ( version(0, 0, 1) )
#define MUSE_BLOCKCHAIN_HARDFORK_VERSION     ( hardfork_version( MUSE_BLOCKCHAIN_VERSION ) )

#ifdef IS_TEST_NET // This is the muse test net mode. Some feature may behave differently
#define MUSE_INIT_PRIVATE_KEY                (fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("init_key"))))
#define MUSE_INIT_PUBLIC_KEY_STR             (std::string( muse::chain::public_key_type(MUSE_INIT_PRIVATE_KEY.get_public_key()) ))
#define MUSE_CHAIN_ID                        (fc::sha256::hash("muse testnet"))
/*
#define VESTS_SYMBOL  (uint64_t(6) | (uint64_t('V') << 8) | (uint64_t('E') << 16) | (uint64_t('S') << 24) | (uint64_t('T') << 32) | (uint64_t('S') << 40)) 
#define MUSE_SYMBOL  (uint64_t(3) | (uint64_t('T') << 8) | (uint64_t('E') << 16) | (uint64_t('S') << 24) | (uint64_t('T') << 32) | (uint64_t('S') << 40)) 
#define MBD_SYMBOL    (uint64_t(3) | (uint64_t('T') << 8) | (uint64_t('B') << 16) | (uint64_t('D') << 24) ) 
#define STMD_SYMBOL   (uint64_t(3) | (uint64_t('T') << 8) | (uint64_t('S') << 16) | (uint64_t('T') << 24) | (uint64_t('D') << 32) ) 
#define NULL_SYMBOL  (uint64_t(3) )
*/
#define BASE_SYMBOL                          "TEST"
#define MUSE_ADDRESS_PREFIX                  "TST"
#define MUSE_SYMBOL_STRING   					(BASE_SYMBOL)
#define MBD_SYMBOL_STRING     					"TBD"

#define MUSE_GENESIS_TIME                    (fc::time_point_sec(1451606400))
#define MUSE_MINING_TIME                     (fc::time_point_sec(1451606400))
#define MUSE_CASHOUT_WINDOW_SECONDS          (60*60) /// 1 hr
#define MUSE_CASHOUT_WINDOW_SECONDS_PRE_HF12 (MUSE_CASHOUT_WINDOW_SECONDS)
#define MUSE_SECOND_CASHOUT_WINDOW           (60*60*24*3) /// 3 days
#define MUSE_MAX_CASHOUT_WINDOW_SECONDS      (60*60*24) /// 1 day
#define MUSE_VOTE_CHANGE_LOCKOUT_PERIOD      (60*10) /// 10 minutes


#define MUSE_ORIGINAL_MIN_ACCOUNT_CREATION_FEE 0
#define MUSE_MIN_ACCOUNT_CREATION_FEE          0

#define MUSE_OWNER_AUTH_RECOVERY_PERIOD                  fc::seconds(60)
#define MUSE_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::seconds(12)
#define MUSE_OWNER_UPDATE_LIMIT                          fc::seconds(0)
#define MUSE_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 1

#elif defined(IS_MUSE_TEST) // This is MUSE's test mode. It's the same behavior as prod, but with different symbols, chain ID, prefixes to make a clear distinction from the prod network. Some minor settings may be adjusted to ease timeouts, limits, etc

#define MUSE_INIT_PUBLIC_KEY_STR             "TST7MY5ZS1e7vhN5msMDUwdc5L6e1nizC6NGQTduZXQooBSD6iK61"
#define MUSE_CHAIN_ID                        (fc::sha256::hash("muse testnet"))
#define BASE_SYMBOL                       "TEST"
#define MUSE_ADDRESS_PREFIX               "TST"
#define MUSE_SYMBOL_STRING   				 (BASE_SYMBOL)
#define MBD_SYMBOL_STRING     				 "TBD"
#define VESTS_SYMBOL_STRING   				 "TVESTS"

#define NULL_SYMBOL  (uint64_t(3) )

#define MUSE_GENESIS_TIME                    (fc::time_point_sec(1458835200))
#define MUSE_MINING_TIME                     (fc::time_point_sec(1458838800))
#define MUSE_CASHOUT_WINDOW_SECONDS_PRE_HF12 (60*60*24)  /// 1 day
#define MUSE_CASHOUT_WINDOW_SECONDS          (60*60*12)  /// 12 hours
#define MUSE_SECOND_CASHOUT_WINDOW           (60*60*24*30) /// 30 days
#define MUSE_MAX_CASHOUT_WINDOW_SECONDS      (60*60*24*14) /// 2 weeks
#define MUSE_VOTE_CHANGE_LOCKOUT_PERIOD      (60*60*2) /// 2 hours

#define MUSE_ORIGINAL_MIN_ACCOUNT_CREATION_FEE  100000
#define MUSE_MIN_ACCOUNT_CREATION_FEE           1

#define MUSE_OWNER_AUTH_RECOVERY_PERIOD                  fc::days(30)
#define MUSE_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::days(1)
#define MUSE_OWNER_UPDATE_LIMIT                          fc::minutes(60)
#define MUSE_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 3186477

#else // This is MUSE's PROD mode.

#define MUSE_INIT_PUBLIC_KEY_STR             "MUSE75fnamVHSMV67JaRPSKbCr8vDwGoCEDeDky8e78JJbNEYcbqv1"
#define MUSE_CHAIN_ID                        (fc::sha256::hash("muse mainchain"))
#define BASE_SYMBOL                          "MUSE"
#define MUSE_ADDRESS_PREFIX                  "MUSE"
#define MUSE_SYMBOL_STRING   (BASE_SYMBOL)
#define VESTS_SYMBOL_STRING   "VESTS"
#define MBD_SYMBOL_STRING     "MBD"

#define NULL_SYMBOL  (uint64_t(3) )

#define MUSE_GENESIS_TIME                    (fc::time_point_sec(1458835200))
#define MUSE_MINING_TIME                     (fc::time_point_sec(1458838800))
#define MUSE_CASHOUT_WINDOW_SECONDS_PRE_HF12 (60*60*24)  /// 1 day
#define MUSE_CASHOUT_WINDOW_SECONDS          (60*60*12)  /// 12 hours
#define MUSE_SECOND_CASHOUT_WINDOW           (60*60*24*30) /// 30 days
#define MUSE_MAX_CASHOUT_WINDOW_SECONDS      (60*60*24*14) /// 2 weeks
#define MUSE_VOTE_CHANGE_LOCKOUT_PERIOD      (60*60*2) /// 2 hours

#define MUSE_ORIGINAL_MIN_ACCOUNT_CREATION_FEE  100000
#define MUSE_MIN_ACCOUNT_CREATION_FEE           1

#define MUSE_OWNER_AUTH_RECOVERY_PERIOD                  fc::days(30)
#define MUSE_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::days(1)
#define MUSE_OWNER_UPDATE_LIMIT                          fc::minutes(60)
#define MUSE_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 3186477

#endif

#define MUSE_MIN_ASSET_SYMBOL_LENGTH            3
#define MUSE_MAX_ASSET_SYMBOL_LENGTH            8

#define MUSE_BLOCK_INTERVAL                  3
#define MUSE_BLOCKS_PER_YEAR                 (365*24*60*60/MUSE_BLOCK_INTERVAL)
#define MUSE_BLOCKS_PER_DAY                  (24*60*60/MUSE_BLOCK_INTERVAL)
#define MUSE_START_VESTING_BLOCK             (MUSE_BLOCKS_PER_DAY * 1)
#define MUSE_START_MINER_VOTING_BLOCK        (MUSE_BLOCKS_PER_DAY * 1)
 
#define MUSE_INIT_MINER_NAME                 "initminer"
#define MUSE_NUM_INIT_MINERS                 1
#define MUSE_INIT_TIME                       (fc::time_point_sec());
#define MUSE_MAX_VOTED_WITNESSES             20
#define MUSE_MAX_RUNNER_WITNESSES            1
#define MUSE_MAX_MINERS (MUSE_MAX_VOTED_WITNESSES+MUSE_MAX_RUNNER_WITNESSES) /// 21 is more than enough
#define MUSE_MAX_VOTED_STREAMING_PLATFORMS      10
#define MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE    10000000

#define MUSE_HARDFORK_REQUIRED_WITNESSES     17 // 17 of the 20 dpos witnesses (19 elected and 1 virtual time) required for hardfork. This guarantees 75% participation on all subsequent rounds.
#define MUSE_MAX_TIME_UNTIL_EXPIRATION       (60*60) // seconds,  aka: 1 hour
#define MUSE_MAX_MEMO_SIZE                   2048
#define MUSE_MAX_PROXY_RECURSION_DEPTH       4
#define MUSE_VESTING_WITHDRAW_INTERVALS      13
#define MUSE_VESTING_WITHDRAW_INTERVAL_SECONDS (60*60*24*7) /// 1 week per interval
#define MUSE_MAX_WITHDRAW_ROUTES             10
#define MUSE_VOTE_REGENERATION_SECONDS       (5*60*60*24) // 5 day
#define MUSE_MAX_VOTE_CHANGES                5
#define MUSE_UPVOTE_LOCKOUT                  (fc::minutes(1))
#define MUSE_REVERSE_AUCTION_WINDOW_SECONDS  (60*30) /// 30 minutes
#define MUSE_MIN_VOTE_INTERVAL_SEC           3

#define MUSE_MIN_ROOT_COMMENT_INTERVAL       (fc::seconds(60*5)) // 5 minutes
#define MUSE_MIN_REPLY_INTERVAL              (fc::seconds(20)) // 20 seconds
#define MUSE_POST_AVERAGE_WINDOW             (60*60*24u) // 1 day
#define MUSE_POST_MAX_BANDWIDTH              (4*MUSE_100_PERCENT) // 2 posts per 1 days, average 1 every 12 hours
#define MUSE_POST_WEIGHT_CONSTANT            (uint64_t(MUSE_POST_MAX_BANDWIDTH) * MUSE_POST_MAX_BANDWIDTH)

#define MUSE_MAX_ACCOUNT_WITNESS_VOTES       30

#define MUSE_100_PERCENT                     10000
#define MUSE_1_PERCENT                       (MUSE_100_PERCENT/100)
#define MUSE_DEFAULT_SBD_INTEREST_RATE       (10*MUSE_1_PERCENT) ///< 10% APR

#define MUSE_MINER_PAY_PERCENT               (MUSE_1_PERCENT) // 1%
#define MUSE_MIN_RATION                      100000
#define MUSE_MAX_RATION_DECAY_RATE           (1000000)
#define MUSE_FREE_TRANSACTIONS_WITH_NEW_ACCOUNT 100

#define MUSE_BANDWIDTH_AVERAGE_WINDOW_SECONDS (60*60*24*7) ///< 1 week
#define MUSE_BANDWIDTH_PRECISION             1000000ll ///< 1 million
#define MUSE_MAX_COMMENT_DEPTH               6

#define MUSE_MAX_RESERVE_RATIO   (20000)


#define MUSE_MINING_REWARD                   asset( 100, MUSE_SYMBOL )

#define MUSE_LIQUIDITY_TIMEOUT_SEC           (fc::seconds(60*60*24*7)) // After one week volume is set to 0
#define MUSE_MIN_LIQUIDITY_REWARD_PERIOD_SEC (fc::seconds(60)) // 1 minute required on books to receive volume
#define MUSE_LIQUIDITY_REWARD_PERIOD_SEC     (60*60)
#define MUSE_LIQUIDITY_REWARD_BLOCKS         (MUSE_LIQUIDITY_REWARD_PERIOD_SEC/MUSE_BLOCK_INTERVAL)
#define MUSE_MIN_LIQUIDITY_REWARD            (asset( 1000000*MUSE_LIQUIDITY_REWARD_BLOCKS, MUSE_SYMBOL )) // Minumum reward to be paid out to liquidity providers
#define MUSE_MIN_CONTENT_REWARD              MUSE_MINING_REWARD
#define MUSE_MIN_CURATE_REWARD               MUSE_MINING_REWARD
#define MUSE_MIN_PRODUCER_REWARD             MUSE_MINING_REWARD
#define MUSE_MIN_POW_REWARD                  MUSE_MINING_REWARD

#define MUSE_ACTIVE_CHALLENGE_FEE            asset( 20000, MUSE_SYMBOL )
#define MUSE_OWNER_CHALLENGE_FEE             asset( 300000, MUSE_SYMBOL )
#define MUSE_ACTIVE_CHALLENGE_COOLDOWN       fc::days(1)
#define MUSE_OWNER_CHALLENGE_COOLDOWN        fc::days(1)

#define MUSE_ASSET_PRECISION                    6
#define MUSE_PAYOUT_TIME_OF_DAY                 2
#define MUSE_MAX_LISTENING_PERIOD               3600

// 5ccc e802 de5f
// int(expm1( log1p( 1 ) / BLOCKS_PER_YEAR ) * 2**MUSE_APR_PERCENT_SHIFT_PER_BLOCK / 100000 + 0.5)
// we use 100000 here instead of 10000 because we end up creating an additional 9x for vesting
#define MUSE_APR_PERCENT_MULTIPLY_PER_BLOCK          ( (uint64_t( 0x5ccc ) << 0x20) \
                                                        | (uint64_t( 0xe802 ) << 0x10) \
                                                        | (uint64_t( 0xde5f )        ) \
                                                        )
// chosen to be the maximal value such that MUSE_APR_PERCENT_MULTIPLY_PER_BLOCK * 2**64 * 100000 < 2**128
#define MUSE_APR_PERCENT_SHIFT_PER_BLOCK             87

#define MUSE_APR_PERCENT_MULTIPLY_PER_ROUND          ( (uint64_t( 0x79cc ) << 0x20 ) \
                                                        | (uint64_t( 0xf5c7 ) << 0x10 ) \
                                                        | (uint64_t( 0x3480 )         ) \
                                                        )

#define MUSE_APR_PERCENT_SHIFT_PER_ROUND             83

// We have different constants for hourly rewards
// i.e. hex(int(math.expm1( math.log1p( 1 ) / HOURS_PER_YEAR ) * 2**MUSE_APR_PERCENT_SHIFT_PER_HOUR / 100000 + 0.5))
#define MUSE_APR_PERCENT_MULTIPLY_PER_HOUR           ( (uint64_t( 0x6cc1 ) << 0x20) \
                                                        | (uint64_t( 0x39a1 ) << 0x10) \
                                                        | (uint64_t( 0x5cbd )        ) \
                                                        )

// chosen to be the maximal value such that MUSE_APR_PERCENT_MULTIPLY_PER_HOUR * 2**64 * 100000 < 2**128
#define MUSE_APR_PERCENT_SHIFT_PER_HOUR              77


#define MUSE_APR_PERCENT_MULTIPLY_PER_DAY            ( (uint64_t( 0x1347 ) << 20 ) \
                                                        | (uint64_t( 0xdcd1 ) << 10 ) \
                                                        | (uint64_t( 0x906D )       ) )

#define MUSE_APR_PERCENT_SHIFT_PER_DAY               73

#define MUSE_CURATE_APR_PERCENT_RESERVE           1
#define MUSE_CONTENT_APR_PERCENT                712
#define MUSE_LIQUIDITY_APR_PERCENT           0
#define MUSE_VESTING_ARP_PERCENT                143
#define MUSE_PRODUCER_APR_PERCENT            95

#define MUSE_CURATION_THRESHOLD1                1000
#define MUSE_CURATION_THRESHOLD2                2000
#define MUSE_CURATION_DURATION                  (uint64_t(14*24*60*60))


#define MUSE_MIN_PAYOUT_SBD                  (asset(20,MBD_SYMBOL))

#define MUSE_MIN_ACCOUNT_NAME_LENGTH          3
#define MUSE_MAX_ACCOUNT_NAME_LENGTH         16

#define MUSE_MIN_PERMLINK_LENGTH             0
#define MUSE_MAX_PERMLINK_LENGTH             256
#define MUSE_MAX_WITNESS_URL_LENGTH          2048
#define MUSE_MAX_STREAMING_PLATFORM_URL_LENGTH          2048

#define MUSE_INIT_SUPPLY                     int64_t(0)
#define MUSE_MAX_SHARE_SUPPLY                int64_t(30000000000000ll)
#define MUSE_MAX_SIG_CHECK_DEPTH             2

#define MUSE_MIN_TRANSACTION_SIZE_LIMIT      1024
#define MUSE_SECONDS_PER_YEAR                (uint64_t(60*60*24*365ll))

#define MUSE_SBD_INTEREST_COMPOUND_INTERVAL_SEC  (60*60*24*30)
#define MUSE_MAX_TRANSACTION_SIZE            (1024*64)
#define MUSE_MIN_BLOCK_SIZE_LIMIT            (MUSE_MAX_TRANSACTION_SIZE)
#define MUSE_MAX_BLOCK_SIZE                  (MUSE_MAX_TRANSACTION_SIZE*MUSE_BLOCK_INTERVAL*2000)
#define MUSE_BLOCKS_PER_HOUR                 (60*60/MUSE_BLOCK_INTERVAL)
#define MUSE_FEED_INTERVAL_BLOCKS            (MUSE_BLOCKS_PER_HOUR)
#define MUSE_FEED_HISTORY_WINDOW             (24*7) /// 7 days * 24 hours per day
#define MUSE_MAX_FEED_AGE                    (fc::days(7))
#define MUSE_MIN_FEEDS                       1 ///(MUSE_MAX_MINERS/3) /// protects the network from conversions before price has been established
#define MUSE_CONVERSION_DELAY                (fc::hours( 3 * 24 + 12 ) )

#define MUSE_MIN_UNDO_HISTORY                10
#define MUSE_MAX_UNDO_HISTORY                10000

#define MUSE_MIN_TRANSACTION_EXPIRATION_LIMIT (MUSE_BLOCK_INTERVAL * 5) // 5 transactions per block

#define MUSE_MAX_INSTANCE_ID                 (uint64_t(-1)>>16)
/** NOTE: making this a power of 2 (say 2^15) would greatly accelerate fee calcs */
#define MUSE_MAX_AUTHORITY_MEMBERSHIP        10
#define MUSE_MAX_ASSET_WHITELIST_AUTHORITIES 10
#define MUSE_MAX_URL_LENGTH                  127

#define GRAPHENE_CURRENT_DB_VERSION             "GPH2.4"

#define MUSE_IRREVERSIBLE_THRESHOLD          (51 * MUSE_1_PERCENT)

/**
 *  Reserved Account IDs with special meaning
 */
///@{
/// Represents the current witnesses
#define MUSE_MINER_ACCOUNT                   "miners"
/// Represents the canonical account with NO authority (nobody can access funds in null account)
#define MUSE_NULL_ACCOUNT                    "null"
/// Represents the canonical account with WILDCARD authority (anybody can access funds in temp account)
#define MUSE_TEMP_ACCOUNT                    "temp"
/// Represents the canonical account for specifying you will vote for directly (as opposed to a proxy)
#define MUSE_PROXY_TO_SELF_ACCOUNT           ""
///@}

#define MUSE_1ST_LEVEL_SCORING_PERCENTAGE 50
#define MUSE_2ST_LEVEL_SCORING_PERCENTAGE 10
