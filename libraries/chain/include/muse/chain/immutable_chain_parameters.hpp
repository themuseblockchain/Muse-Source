#pragma once

#include <fc/reflect/reflect.hpp>

#include <cstdint>

#include <muse/chain/config.hpp>

namespace muse { namespace chain {

struct immutable_chain_parameters
{
   uint16_t min_committee_member_count = MUSE_DEFAULT_MIN_COMMITTEE_MEMBER_COUNT;
   uint16_t min_witness_count = MUSE_DEFAULT_MIN_WITNESS_COUNT;
   uint32_t num_special_accounts = 0;
   uint32_t num_special_assets = 0;
};

} } // muse::chain

FC_REFLECT( muse::chain::immutable_chain_parameters,
   (min_committee_member_count)
   (min_witness_count)
   (num_special_accounts)
   (num_special_assets)
)
