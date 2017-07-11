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
#pragma once
#include <muse/chain/protocol/types.hpp>
#include <muse/chain/protocol/address.hpp>
#include <graphene/db/generic_index.hpp>


namespace muse { namespace chain {

class balance_object : public abstract_object<balance_object> {
public:
   static const uint8_t space_id = implementation_ids;
   static const uint8_t type_id = impl_balance_object_type;

   address owner;
   asset balance;
   time_point_sec last_claim_date;
};

struct by_owner;

typedef multi_index_container <
balance_object,
indexed_by<
      ordered_unique < tag < by_id>, member<object, object_id_type, &object::id>>,
      ordered_non_unique <tag<by_owner>, member<balance_object, address, &balance_object::owner>>
>
>
balance_multi_index_type;

typedef generic_index <balance_object, balance_multi_index_type> balance_index;

} }// muse::chain

FC_REFLECT_DERIVED( muse::chain::balance_object, (graphene::db::object),
(owner)(balance)(last_claim_date) )