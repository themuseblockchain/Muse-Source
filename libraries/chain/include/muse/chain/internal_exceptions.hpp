#pragma once

#include <fc/exception/exception.hpp>
#include <muse/chain/exceptions.hpp>

#define MUSE_DECLARE_INTERNAL_EXCEPTION( exc_name, seqnum, msg )  \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      internal_ ## exc_name,                                          \
      muse::chain::internal_exception,                            \
      3990000 + seqnum,                                               \
      msg                                                             \
      )

namespace muse { namespace chain {

FC_DECLARE_DERIVED_EXCEPTION( internal_exception, muse::chain::chain_exception, 3990000, "internal exception" )

MUSE_DECLARE_INTERNAL_EXCEPTION( verify_auth_max_auth_exceeded, 1, "Exceeds max authority fan-out" )
MUSE_DECLARE_INTERNAL_EXCEPTION( verify_auth_account_not_found, 2, "Auth account not found" )

} } // muse::chain
