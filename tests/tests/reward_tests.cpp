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

#include <muse/chain/compound.hpp>

#include "../common/database_fixture.hpp"

using namespace muse::chain;
using namespace graphene::db;

BOOST_AUTO_TEST_SUITE( reward_tests )

/**
 * According to cob, the inflation
 * "should be a total of about 9.5% per year. But from that 9.5% 10% to
 *  wtnesses, 15% vests and 75% content"
 * Meaning witnesses receive 0.95% inflation, vests 1.425% (rounded to 1.43)
 * and content 7.125% (rounded to 7.12%).
 *
 * As a rough estimate we assume that the average supply over the course of
 * the year is 1.0475 times the supply at the beginning (in reality it's
 * slightly lower).
 */

BOOST_AUTO_TEST_CASE( witness_reward_test )
{
    int64_t supply = 18000000LL * 1000000;
    int64_t expected_witness_reward = supply * .0095;

    int64_t avg_supply = 1.0475 * supply;
    int64_t avg_witness_reward = calc_percent_reward_per_block<MUSE_PRODUCER_APR_PERCENT>( avg_supply ).value;
    int64_t witness_reward_per_year = avg_witness_reward * MUSE_BLOCKS_PER_YEAR;

    std::cerr << "Expected witness reward: " << expected_witness_reward
              << ". actual: " << witness_reward_per_year << std::endl;
    // check estimate is within 5% of expected
    BOOST_CHECK( expected_witness_reward * .95 < witness_reward_per_year );
    BOOST_CHECK( witness_reward_per_year < expected_witness_reward * 1.05 );
}

BOOST_AUTO_TEST_CASE( vesting_reward_test )
{
    int64_t supply = 18000000LL * 1000000;
    int64_t expected_vesting_reward = supply * .01425;

    int64_t avg_supply = 1.0475 * supply;
    int64_t avg_vesting_reward = calc_percent_reward_per_block<MUSE_VESTING_ARP_PERCENT>( avg_supply ).value;
    int64_t vesting_reward_per_year = avg_vesting_reward * MUSE_BLOCKS_PER_YEAR;

    std::cerr << "Expected vesting reward: " << expected_vesting_reward
              << ". actual: " << vesting_reward_per_year << std::endl;
    // check estimate is within 5% of expected
    BOOST_CHECK( expected_vesting_reward * .95 < vesting_reward_per_year );
    BOOST_CHECK( vesting_reward_per_year < expected_vesting_reward * 1.05 );
}

BOOST_AUTO_TEST_CASE( content_reward_test )
{
    int64_t supply = 18000000LL * 1000000;
    int64_t expected_content_reward = supply * .07125;

    int64_t avg_supply = 1.0475 * supply;
    int64_t avg_content_reward = calc_percent_reward_per_day<MUSE_CONTENT_APR_PERCENT>( avg_supply ).value;
    int64_t content_reward_per_year = avg_content_reward * 365;

    std::cerr << "Expected content reward: " << expected_content_reward
              << ". actual: " << content_reward_per_year << std::endl;
    // check estimate is within 5% of expected
    BOOST_CHECK( expected_content_reward * .95 < content_reward_per_year );
    BOOST_CHECK( content_reward_per_year < expected_content_reward * 1.05 );
}

/**
 * Verify block producer reward
 */
BOOST_AUTO_TEST_CASE( producer_test )
{
}

BOOST_AUTO_TEST_SUITE_END()
