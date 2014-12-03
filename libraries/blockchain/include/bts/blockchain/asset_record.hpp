#pragma once
#include <bts/blockchain/types.hpp>
#include <bts/blockchain/withdraw_types.hpp>
#include <bts/blockchain/transaction.hpp>

namespace bts { namespace blockchain {

   struct asset_record
   {
      enum
      {
          market_issued_asset = -2,
      };

      share_type available_shares()const;

      bool can_issue( const asset& amount )const;
      bool can_issue( const share_type& amount )const;

      bool is_null()const;
      /** the asset is issued by the market and not by any user */
      bool is_market_issued()const;

      bool is_chip_asset() const;

      bool is_retractable()const { return !is_market_issued() && retractable; }
      bool is_restricted()const { return !is_market_issued() && restricted; }
      asset_record make_null()const;

      uint64_t get_precision()const;

      asset_id_type       id;
      std::string         symbol;
      std::string         name;
      std::string         description;
      fc::variant         public_data;
      account_id_type     issuer_account_id;
      uint64_t            precision = 0;
      fc::time_point_sec  registration_date;
      fc::time_point_sec  last_update;
      share_type          current_collateral = 0;
      share_type          current_share_supply = 0;
      share_type          maximum_share_supply = 0;
      share_type          collected_fees = 0;
      /**
       * A restricted asset can only be held/controlled by keys
       * on the authorized list.
       */
      bool                restricted  = false;
      /**
       * Asset is retractable by the issuer, makes the asset authority
       * and implicit co-signer on all balances that involve this asset.
       */
      bool                retractable = true;

      /**
       *  The issuer can specify a transaction fee (of the asset type) 
       *  that will be paid to the issuer with every transaction that
       *  references this asset type.
       */
      share_type          transaction_fee = 0;
      multisig_meta_info  authority;

      /** reserved for future extensions */
      vector<char>        reserved; 
   };
   typedef fc::optional<asset_record> oasset_record;

} } // bts::blockchain

FC_REFLECT( bts::blockchain::asset_record,
            (id)
            (symbol)
            (name)
            (description)
            (public_data)
            (issuer_account_id)
            (precision)
            (registration_date)
            (last_update)
            (current_collateral)
            (current_share_supply)
            (maximum_share_supply)
            (collected_fees)
            (restricted)
            (retractable)
            (transaction_fee)
            (authority)
           )
