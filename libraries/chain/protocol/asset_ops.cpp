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
#include <muse/chain/protocol/asset_ops.hpp>
#include <locale>

static const std::locale& c_locale = std::locale::classic();

namespace muse { namespace chain {

/**
 *  Valid symbols can contain [A-Z0-9], and '.'
 *  They must start with [A, Z]
 *  They must end with [A, Z]
 *  They can contain a maximum of one '.'
 */
bool is_valid_symbol( const string& symbol )
{
    if( symbol.size() < MUSE_MIN_ASSET_SYMBOL_LENGTH )
        return false;

    if( symbol.size() > MUSE_MAX_ASSET_SYMBOL_LENGTH )
        return false;

    if( !isalpha( symbol.front(), c_locale ) )
        return false;

    if( !isalpha( symbol.back(), c_locale ) )
        return false;

    bool dot_already_present = false;
    for( const auto c : symbol )
    {
        if( (isalpha( c, c_locale ) && isupper( c, c_locale )) || isdigit(c, c_locale) )
            continue;

        if( c == '.' )
        {
            if( dot_already_present )
                return false;

            dot_already_present = true;
            continue;
        }

        return false;
    }

    return true;
}



void  asset_create_operation::validate()const
{
   FC_ASSERT( is_valid_symbol(symbol) );
   common_options.validate();

   FC_ASSERT(precision <= 12);
}

void asset_update_operation::validate()const
{
   if( new_issuer )
      FC_ASSERT(issuer != *new_issuer);
   new_options.validate();
}

void asset_reserve_operation::validate()const
{
   FC_ASSERT( amount_to_reserve.amount.value <= MUSE_MAX_SHARE_SUPPLY );
   FC_ASSERT( amount_to_reserve.amount.value > 0 );
}

void asset_issue_operation::validate()const
{
   FC_ASSERT( asset_to_issue.amount.value <= MUSE_MAX_SHARE_SUPPLY );
   FC_ASSERT( asset_to_issue.amount.value > 0 );
}

void asset_options::validate()const
{
   FC_ASSERT( max_supply > 0 );
   FC_ASSERT( max_supply <= MUSE_MAX_SHARE_SUPPLY );
   FC_ASSERT( market_fee_percent <= MUSE_100_PERCENT );
   FC_ASSERT( max_market_fee >= 0 && max_market_fee <= MUSE_MAX_SHARE_SUPPLY );
   // There must be no high bits in permissions whose meaning is not known.
   FC_ASSERT( !(issuer_permissions & ~ ASSET_ISSUER_PERMISSION_MASK) );

}

} } // namespace muse::chain
