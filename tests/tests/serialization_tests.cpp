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

#include <muse/chain/base_objects.hpp>
#include <muse/chain/database.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/variant.hpp>

#include "../common/database_fixture.hpp"

#include <cmath>

using namespace muse::chain;

BOOST_FIXTURE_TEST_SUITE( serialization_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( serialization_raw_test )
{
   try {
      ACTORS( (alice)(bob) )
      transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = asset(100,MUSE_SYMBOL);

      trx.operations.push_back( op );
      auto packed = fc::raw::pack( trx );
      signed_transaction unpacked = fc::raw::unpack<signed_transaction>(packed);
      unpacked.validate();
      BOOST_CHECK( trx.digest() == unpacked.digest() );
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}
BOOST_AUTO_TEST_CASE( serialization_json_test )
{
   try {
      ACTORS( (alice)(bob) )
      transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = asset(100,MUSE_SYMBOL);


      fc::variant test(op.amount);
      auto tmp = test.as<asset>();

      trx.operations.push_back( op );
      fc::variant packed(trx);
      signed_transaction unpacked = packed.as<signed_transaction>();
      unpacked.validate();
      BOOST_CHECK( trx.digest() == unpacked.digest() );
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( asset_test )
{
   try
   {
      BOOST_CHECK_EQUAL( asset().decimals(), MUSE_ASSET_PRECISION );
      BOOST_CHECK_EQUAL( asset().asset_id, MUSE_SYMBOL );
      BOOST_CHECK_EQUAL( asset().to_string(), "0.000000 2.28.0" );

      BOOST_TEST_MESSAGE( "Asset Test" );
      asset muse = asset::from_string( "123.456 2.28.0" );
      asset mbd = asset::from_string( "654.321 2.28.2" );
      asset tmp = asset::from_string( "0.456 2.28.0" );
      BOOST_CHECK_EQUAL( tmp.amount.value, 456000 );
      tmp = asset::from_string( "0.056 2.28.0" );
      BOOST_CHECK_EQUAL( tmp.amount.value, 56000 );

      BOOST_CHECK( std::abs( muse.to_real() - 123.456 ) < 0.0005 );
      BOOST_CHECK_EQUAL( muse.decimals(), MUSE_ASSET_PRECISION );
      BOOST_CHECK_EQUAL( muse.asset_id, MUSE_SYMBOL );
      BOOST_CHECK_EQUAL( muse.to_string(), "123.456000 2.28.0" );
      BOOST_CHECK_EQUAL( asset(50, MUSE_SYMBOL).to_string(), "0.000050 2.28.0" );
      BOOST_CHECK_EQUAL( asset(50000000, MUSE_SYMBOL).to_string(), "50.000000 2.28.0" );

      BOOST_CHECK( std::abs( mbd.to_real() - 654.321 ) < 0.0005 );
      BOOST_CHECK_EQUAL( mbd.decimals(), MUSE_ASSET_PRECISION );
      BOOST_CHECK_EQUAL( mbd.asset_id, MBD_SYMBOL );
      BOOST_CHECK_EQUAL( mbd.to_string(), "654.321000 2.28.2" );
      BOOST_CHECK_EQUAL( asset(50000, MBD_SYMBOL).to_string(), "0.050000 2.28.2" );
      BOOST_CHECK_EQUAL( asset(50000000, MBD_SYMBOL).to_string(), "50.000000 2.28.2" );

      muse = asset::from_string( "123.456789 2.28.1" );
      BOOST_CHECK_EQUAL( muse.asset_id, VESTS_SYMBOL );
      BOOST_CHECK_EQUAL( muse.amount.value, 123456789 );

      muse = asset::from_string( "123.45678 2.28.1" );
      BOOST_CHECK_EQUAL( muse.asset_id, VESTS_SYMBOL );
      BOOST_CHECK_EQUAL( muse.amount.value, 123456780 );

      muse = asset::from_string( "123.4567 2.28.1" );
      BOOST_CHECK_EQUAL( muse.asset_id, VESTS_SYMBOL );
      BOOST_CHECK_EQUAL( muse.amount.value, 123456700 );

      muse = asset::from_string( "123.456 2.28.1" );
      BOOST_CHECK_EQUAL( muse.asset_id, VESTS_SYMBOL );
      BOOST_CHECK_EQUAL( muse.amount.value, 123456000 );

      muse = asset::from_string( "123.45 2.28.1" );
      BOOST_CHECK_EQUAL( muse.asset_id, VESTS_SYMBOL );
      BOOST_CHECK_EQUAL( muse.amount.value, 123450000 );

      muse = asset::from_string( "123.4 2.28.1" );
      BOOST_CHECK_EQUAL( muse.asset_id, VESTS_SYMBOL );
      BOOST_CHECK_EQUAL( muse.amount.value, 123400000 );

      muse = asset::from_string( "123.0 2.28.1" );
      BOOST_CHECK_EQUAL( muse.asset_id, VESTS_SYMBOL );
      BOOST_CHECK_EQUAL( muse.amount.value, 123000000 );

      muse = asset::from_string( "123 2.28.1" );
      BOOST_CHECK_EQUAL( muse.asset_id, VESTS_SYMBOL );
      BOOST_CHECK_EQUAL( muse.amount.value, 123000000 );

      mbd = asset::from_string( "120.0000091 2.28.2" );
      BOOST_CHECK_EQUAL( mbd.asset_id, MBD_SYMBOL );
      BOOST_CHECK_EQUAL( mbd.amount.value, 120000009 );
      BOOST_CHECK_EQUAL( mbd.to_string(), "120.000009 2.28.2" );

      mbd = asset( 120000090, MBD_SYMBOL );
      BOOST_CHECK_EQUAL( mbd.to_string(), "120.000090 2.28.2" );

      mbd = asset( 120000900, MBD_SYMBOL );
      BOOST_CHECK_EQUAL( mbd.to_string(), "120.000900 2.28.2" );

      mbd = asset( 120009000, MBD_SYMBOL );
      BOOST_CHECK_EQUAL( mbd.to_string(), "120.009000 2.28.2" );

      mbd = asset( 120090000, MBD_SYMBOL );
      BOOST_CHECK_EQUAL( mbd.to_string(), "120.090000 2.28.2" );

      mbd = asset( 120900000, MBD_SYMBOL );
      BOOST_CHECK_EQUAL( mbd.to_string(), "120.900000 2.28.2" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( json_tests )
{
   try {
   auto var = fc::json::variants_from_string( "10.6 " );
   var = fc::json::variants_from_string( "10.5" );
   } catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( extended_private_key_type_test )
{
   try
   {
     fc::ecc::extended_private_key key = fc::ecc::extended_private_key( fc::ecc::private_key::generate(),
                                                                       fc::sha256(),
                                                                       0, 0, 0 );
      extended_private_key_type type = extended_private_key_type( key );
      std::string packed = std::string( type );
      extended_private_key_type unpacked = extended_private_key_type( packed );
      BOOST_CHECK( type == unpacked );
   } catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( extended_public_key_type_test )
{
   try
   {
      fc::ecc::extended_public_key key = fc::ecc::extended_public_key( fc::ecc::private_key::generate().get_public_key(),
                                                                       fc::sha256(),
                                                                       0, 0, 0 );
      extended_public_key_type type = extended_public_key_type( key );
      std::string packed = std::string( type );
      extended_public_key_type unpacked = extended_public_key_type( packed );
      BOOST_CHECK( type == unpacked );
   } catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( version_test )
{
   try
   {
      BOOST_REQUIRE_EQUAL( string( version( 1, 2, 3) ), "1.2.3" );

      fc::variant ver_str( "3.0.0" );
      version ver;
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version( 3, 0 , 0 ) );

      ver_str = fc::variant( "0.0.0" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version() );

      ver_str = fc::variant( "1.0.1" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version( 1, 0, 1 ) );

      ver_str = fc::variant( "1_0_1" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version( 1, 0, 1 ) );

      ver_str = fc::variant( "12.34.56" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == version( 12, 34, 56 ) );

      ver_str = fc::variant( "256.0.0" );
      MUSE_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::assert_exception );

      ver_str = fc::variant( "0.256.0" );
      MUSE_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::assert_exception );

      ver_str = fc::variant( "0.0.65536" );
      MUSE_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::assert_exception );

      ver_str = fc::variant( "1.0" );
      MUSE_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::assert_exception );

      ver_str = fc::variant( "1.0.0.1" );
      MUSE_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( hardfork_version_test )
{
   try
   {
      BOOST_REQUIRE_EQUAL( string( hardfork_version( 1, 2 ) ), "1.2.0" );

      fc::variant ver_str( "3.0.0" );
      hardfork_version ver;
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 3, 0 ) );

      ver_str = fc::variant( "0.0.0" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version() );

      ver_str = fc::variant( "1.0.0" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 1, 0 ) );

      ver_str = fc::variant( "1_0_0" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 1, 0 ) );

      ver_str = fc::variant( "12.34.00" );
      fc::from_variant( ver_str, ver );
      BOOST_REQUIRE( ver == hardfork_version( 12, 34 ) );

      ver_str = fc::variant( "256.0.0" );
      MUSE_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::assert_exception );

      ver_str = fc::variant( "0.256.0" );
      MUSE_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::assert_exception );

      ver_str = fc::variant( "0.0.1" );
      MUSE_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::assert_exception );

      ver_str = fc::variant( "1.0" );
      MUSE_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::assert_exception );

      ver_str = fc::variant( "1.0.0.1" );
      MUSE_REQUIRE_THROW( fc::from_variant( ver_str, ver ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_SUITE_END()
