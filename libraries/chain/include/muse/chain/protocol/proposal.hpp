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
#include <muse/chain/protocol/base.hpp>

namespace muse { namespace chain { 
   /**
     * @defgroup proposed_transactions  The Muse Transaction Proposal Protocol
     * @ingroup operations
     *
     * Muse allows users to propose a transaction which requires approval of multiple accounts in order to execute.
     * The user proposes a transaction using proposal_create_operation, then signatory accounts use
     * proposal_update_operations to add or remove their approvals from this operation. When a sufficient number of
     * approvals have been granted, the operations in the proposal are used to create a virtual transaction which is
     * subsequently evaluated. Even if the transaction fails, the proposal will be kept until the expiration time, at
     * which point, if sufficient approval is granted, the transaction will be evaluated a final time. This allows
     * transactions which will not execute successfully until a given time to still be executed through the proposal
     * mechanism. The first time the proposed transaction succeeds, the proposal will be regarded as resolved, and all
     * future updates will be invalid.
     *
     * The proposal system allows for arbitrarily complex or recursively nested authorities. If a recursive authority
     * (i.e. an authority which requires approval of 'nested' authorities on other accounts) is required for a
     * proposal, then a second proposal can be used to grant the nested authority's approval. That is, a second
     * proposal can be created which, when sufficiently approved, adds the approval of a nested authority to the first
     * proposal. This multiple-proposal scheme can be used to acquire approval for an arbitrarily deep authority tree.
     *
     * Note that at any time, a proposal can be approved in a single transaction if sufficient signatures are available
     * on the proposal_update_operation, as long as the authority tree to approve the proposal does not exceed the
     * maximum recursion depth. In practice, however, it is easier to use proposals to acquire all approvals, as this
     * leverages on-chain notification of all relevant parties that their approval is required. Off-chain
     * multi-signature approval requires some off-chain mechanism for acquiring several signatures on a single
     * transaction. This off-chain synchronization can be avoided using proposals.
     * @{
     */
   /**
    * op_wrapper is used to get around the circular definition of operation and proposals that contain them.
    */
   struct op_wrapper;
   /**
    * @brief The proposal_create_operation creates a transaction proposal, for use in multi-sig scenarios
    * @ingroup operations
    *
    * Creates a transaction proposal. The operations which compose the transaction are listed in order in proposed_ops,
    * and expiration_time specifies the time by which the proposal must be accepted or it will fail permanently. The
    * expiration_time cannot be farther in the future than the maximum expiration time set in the global properties
    * object.
    */
   struct proposal_create_operation : public base_operation
   {

       vector<op_wrapper> proposed_ops;
       time_point_sec     expiration_time;
       optional<uint32_t> review_period_seconds;
       extensions_type    extensions;

       void            validate()const;
   };

   /**
    * @brief The proposal_update_operation updates an existing transaction proposal
    * @ingroup operations
    *
    * This operation allows accounts to add or revoke approval of a proposed transaction. Signatures sufficient to
    * satisfy the authority of each account in approvals are required on the transaction containing this operation.
    *
    * If an account with a multi-signature authority is listed in approvals_to_add or approvals_to_remove, either all
    * required signatures to satisfy that account's authority must be provided in the transaction containing this
    * operation, or a secondary proposal must be created which contains this operation.
    *
    * NOTE: If the proposal requires only an account's active authority, the account must not update adding its owner
    * authority's approval. This is considered an error. An owner approval may only be added if the proposal requires
    * the owner's authority.
    *
    * If an account's owner and active authority are both required, only the owner authority may approve. An attempt to
    * add or remove active authority approval to such a proposal will fail.
    */
   struct proposal_update_operation : public base_operation
   {
      proposal_id_type           proposal;
      flat_set<string>  active_approvals_to_add;
      flat_set<string>  active_approvals_to_remove;
      flat_set<string>  owner_approvals_to_add;
      flat_set<string>  owner_approvals_to_remove;
      flat_set<public_key_type>  key_approvals_to_add;
      flat_set<public_key_type>  key_approvals_to_remove;
      extensions_type            extensions;

      void            validate()const;
      void get_required_authorities( vector<authority>& )const;
      void get_required_active_authorities( flat_set<string>& )const;
      void get_required_owner_authorities( flat_set<string>& )const;
   };

   /**
    * @brief The proposal_delete_operation deletes an existing transaction proposal
    * @ingroup operations
    *
    * This operation allows the early veto of a proposed transaction. It may be used by any account which is a required
    * authority on the proposed transaction, when that account's holder feels the proposal is ill-advised and he decides
    * he will never approve of it and wishes to put an end to all discussion of the issue. Because he is a required
    * authority, he could simply refuse to add his approval, but this would leave the topic open for debate until the
    * proposal expires. Using this operation, he can prevent any further breath from being wasted on such an absurd
    * proposal.
    */
   struct proposal_delete_operation : public base_operation
   {
      bool              using_owner_authority = false;
      proposal_id_type  proposal;
      extensions_type   extensions;

      void       validate()const;
   };
   ///@}
   
}} // muse::chain


FC_REFLECT( muse::chain::proposal_create_operation, (expiration_time)
            (proposed_ops)(review_period_seconds)(extensions) )
FC_REFLECT( muse::chain::proposal_update_operation, (proposal)
            (active_approvals_to_add)(active_approvals_to_remove)(owner_approvals_to_add)(owner_approvals_to_remove)
            (key_approvals_to_add)(key_approvals_to_remove)(extensions) )
FC_REFLECT( muse::chain::proposal_delete_operation, (using_owner_authority)(proposal)(extensions) )
