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
#include <muse/chain/protocol/asset.hpp>
#include <muse/chain/config.hpp>

namespace muse { namespace chain { 

   bool is_valid_symbol( const string& symbol );


   enum asset_issuer_permission_flags
   {
      charge_market_fee    = 0x01, /**< an issuer-specified percentage of all market trades in this asset is paid to the issuer */
      white_list           = 0x02, /**< accounts must be whitelisted in order to hold this asset */
      override_authority   = 0x04, /**< issuer may transfer asset back to himself */
      transfer_restricted  = 0x08, /**< require the issuer to be one party to every transfer */
      disable_force_settle = 0x10, /**< disable force settling */
      global_settle        = 0x20, /**< allow the bitasset issuer to force a global settling -- this may be set in permissions, but not flags */
      disable_confidential = 0x40, /**< allow the asset to be used with confidential transactions */
   };

   const static uint32_t ASSET_ISSUER_PERMISSION_MASK = charge_market_fee|white_list|override_authority|transfer_restricted|disable_force_settle|global_settle|disable_confidential;
   const static uint32_t UIA_ASSET_ISSUER_PERMISSION_MASK = charge_market_fee|transfer_restricted|disable_confidential;

   /**
    * @brief The asset_options struct contains options available on all assets in the network
    *
    * @note Changes to this struct will break protocol compatibility
    */
   struct asset_options {
      /// The maximum supply of this asset which may exist at any given time. This can be as large as
      /// GRAPHENE_MAX_SHARE_SUPPLY
      share_type max_supply = MUSE_MAX_SHARE_SUPPLY;
      /// When this asset is traded on the markets, this percentage of the total traded will be exacted and paid
      /// to the issuer. This is a fixed point value, representing hundredths of a percent, i.e. a value of 100
      /// in this field means a 1% fee is charged on market trades of this asset.
      uint16_t market_fee_percent = 0;
      /// Market fees calculated as @ref market_fee_percent of the traded volume are capped to this value
      share_type max_market_fee = MUSE_MAX_SHARE_SUPPLY;

      /// The flags which the issuer has permission to update. See @ref asset_issuer_permission_flags
      uint16_t issuer_permissions = UIA_ASSET_ISSUER_PERMISSION_MASK;
      /// The currently active flags on this permission. See @ref asset_issuer_permission_flags
      uint16_t flags = 0;

      /**
       * data that describes the meaning/purpose of this asset, fee will be charged proportional to
       * size of description.
       */
      string description;
      extensions_type extensions;

      /// Perform internal consistency checks.
      /// @throws fc::exception if any check fails
      void validate()const;
   };

  /**
    * @ingroup operations
    */
   struct asset_create_operation : public base_operation
   {

      /// This account must sign and pay the fee for this operation. Later, this account may update the asset
      string                  issuer;
      /// The ticker symbol of this asset
      string                  symbol;
      /// Number of digits to the right of decimal point, must be less than or equal to 12
      uint8_t                 precision = MUSE_ASSET_PRECISION;

      /// Options common to all assets.
      ///
      /// @note common_options.core_exchange_rate technically needs to store the asset ID of this new asset. Since this
      /// ID is not known at the time this operation is created, create this price as though the new asset has instance
      /// ID 1, and the chain will overwrite it with the new asset's ID.
      asset_options              common_options;
      extensions_type extensions;

      void            validate()const;
      void get_required_active_authorities(  flat_set<string>& a )const{ a.insert(issuer); }
   };

   /**
    * @brief Update options common to all assets
    * @ingroup operations
    *
    * There are a number of options which all assets in the network use. These options are enumerated in the @ref
    * asset_options struct. This operation is used to update these options for an existing asset.
    *
    * @note This operation cannot be used to update BitAsset-specific options. For these options, use @ref
    * asset_update_bitasset_operation instead.
    *
    * @pre @ref issuer SHALL be an existing account and MUST match asset_object::issuer on @ref asset_to_update
    * @pre @ref fee SHALL be nonnegative, and @ref issuer MUST have a sufficient balance to pay it
    * @pre @ref new_options SHALL be internally consistent, as verified by @ref validate()
    * @post @ref asset_to_update will have options matching those of new_options
    */
   struct asset_update_operation : public base_operation
   {


      asset_update_operation(){}

      string          issuer;
      asset_id_type   asset_to_update;

      /// If the asset is to be given a new issuer, specify his ID here.
      optional<string>            new_issuer;
      asset_options               new_options;
      extensions_type             extensions;

      void            validate()const;
      void get_required_active_authorities(  flat_set<string>& a )const{ a.insert(issuer); }
   };

   /**
    * @ingroup operations
    */
   struct asset_issue_operation : public base_operation
   {


      string           issuer; ///< Must be asset_to_issue->asset_id->issuer
      asset            asset_to_issue;
      string           issue_to_account;


      extensions_type      extensions;

      void            validate()const;
      void get_required_active_authorities(  flat_set<string>& a )const{ a.insert(issuer); }
   };

   /**
    * @brief used to take an asset out of circulation, returning to the issuer
    * @ingroup operations
    *
    * @note You cannot use this operation on market-issued assets.
    */
   struct asset_reserve_operation : public base_operation
   {

      string            issuer;
      string            payer;
      asset             amount_to_reserve;
      extensions_type   extensions;

      void              validate()const;
      void get_required_active_authorities(  flat_set<string>& a )const{ a.insert(issuer); }
   };

} } // muse::chain

FC_REFLECT( muse::chain::asset_options,
            (max_supply)
            (market_fee_percent)
            (max_market_fee)
            (issuer_permissions)
            (flags)
            (description)
            (extensions)
          )



FC_REFLECT( muse::chain::asset_create_operation,
            (issuer)
            (symbol)
            (precision)
            (common_options)
            (extensions)
          )
FC_REFLECT( muse::chain::asset_update_operation,
            (issuer)
            (asset_to_update)
            (new_issuer)
            (new_options)
            (extensions)
          )
FC_REFLECT( muse::chain::asset_issue_operation,
            (issuer)(asset_to_issue)(issue_to_account)(extensions) )
FC_REFLECT( muse::chain::asset_reserve_operation,
            (issuer)(payer)(amount_to_reserve)(extensions) )

