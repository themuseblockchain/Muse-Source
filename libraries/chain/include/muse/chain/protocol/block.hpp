#pragma once
#include <muse/chain/protocol/block_header.hpp>
#include <muse/chain/protocol/transaction.hpp>

namespace muse { namespace chain {

   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root()const;
      vector<signed_transaction> transactions;
   };

} } // muse::chain

FC_REFLECT_DERIVED( muse::chain::signed_block, (muse::chain::signed_block_header), (transactions) )
