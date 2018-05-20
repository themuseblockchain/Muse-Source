#pragma once
#include <muse/chain/protocol/base.hpp>
#include <muse/chain/protocol/base_operations.hpp>
#include <muse/chain/protocol/asset_ops.hpp>
#include <muse/chain/protocol/proposal.hpp>
#include <muse/chain/protocol/muse_operations.hpp>

namespace muse { namespace chain {

   /** NOTE: do not change the order of any operations prior to the virtual operations
    * or it will trigger a hardfork.
    */
   typedef fc::static_variant<
            vote_operation,
            content_operation,
            content_update_operation,
            content_approve_operation,
            content_disable_operation,

            transfer_operation,
            transfer_to_vesting_operation,
            withdraw_vesting_operation,

            limit_order_create_operation,
            limit_order_create2_operation,
            limit_order_cancel_operation,

            feed_publish_operation,
            convert_operation,

            account_create_operation,
            account_update_operation,

            witness_update_operation,
            account_witness_vote_operation,
            account_witness_proxy_operation,

            streaming_platform_update_operation,
            account_streaming_platform_vote_operation,
            streaming_platform_report_operation,

            asset_create_operation,
            asset_update_operation,
            asset_issue_operation,
            asset_reserve_operation,

            custom_operation,

            report_over_production_operation,

            custom_json_operation,
            set_withdraw_vesting_route_operation,
            challenge_authority_operation,
            prove_authority_operation,
            request_account_recovery_operation,
            recover_account_operation,
            change_recovery_account_operation,
            escrow_transfer_operation,
            escrow_dispute_operation,
            escrow_release_operation,

            //proposals
            proposal_create_operation,
            proposal_update_operation,
            proposal_delete_operation,

            /// virtual operations below this point
            fill_convert_request_operation,
            playing_reward_operation,
            content_reward_operation,
            curate_reward_operation,
            liquidity_reward_operation,
            interest_operation,
            fill_vesting_withdraw_operation,
            fill_order_operation,

            friendship_operation,
            unfriend_operation,

            balance_claim_operation

         > operation;

   void operation_get_required_authorities( const operation& op,
                                            flat_set<string>& active,
                                            flat_set<string>& owner,
                                            flat_set<string>& basic,
                                            flat_set<string>& master_content,
                                            flat_set<string>& comp_content,
                                            vector<authority>&  other );

   void operation_validate( const operation& op );

   struct op_wrapper
   {
      public:
         op_wrapper(const operation& op = operation()):op(op){}
         operation op;
   };

   bool is_market_operation( const operation& op );
   bool is_proposal_operation( const operation& op );

} } // muse::chain

namespace fc {
    void to_variant( const muse::chain::operation& var,  fc::variant& vo, uint32_t max_depth );
    void from_variant( const fc::variant& var,  muse::chain::operation& vo, uint32_t max_depth );
}

FC_REFLECT_TYPENAME( muse::chain::operation )
FC_REFLECT( muse::chain::op_wrapper, (op) )
