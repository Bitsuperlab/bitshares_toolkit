#include <bts/blockchain/chain_interface.hpp>
#include <bts/blockchain/exceptions.hpp>
#include <bts/blockchain/market_operations.hpp>

namespace bts { namespace blockchain {

   /**
    *  If the amount is negative then it will withdraw/cancel the bid assuming
    *  it is signed by the owner and there is sufficient funds.
    *
    *  If the amount is positive then it will add funds to the bid.
    */
   void bid_operation::evaluate( transaction_evaluation_state& eval_state )
   { try {
      if( this->bid_index.order_price == price() )
         FC_CAPTURE_AND_THROW( zero_price, (bid_index.order_price) );

      auto owner = this->bid_index.owner;
      if( !eval_state.check_signature( owner ) )
         FC_CAPTURE_AND_THROW( missing_signature, (bid_index.owner) );

      asset delta_amount  = this->get_amount();

      eval_state.validate_asset( delta_amount );

      auto current_bid   = eval_state._current_state->get_bid_record( this->bid_index );

      if( this->amount == 0 ) FC_CAPTURE_AND_THROW( zero_amount );
      if( this->amount <  0 ) // withdraw
      {
          if( NOT current_bid )
             FC_CAPTURE_AND_THROW( unknown_market_order, (bid_index) );

          if( llabs(this->amount) > current_bid->balance )
             FC_CAPTURE_AND_THROW( insufficient_funds, (amount)(current_bid->balance) );

          // add the delta amount to the eval state that we withdrew from the bid
          eval_state.add_balance( -delta_amount );
      }
      else // this->amount > 0 - deposit
      {
          if( NOT current_bid )  // then initialize to 0
            current_bid = order_record();
          // sub the delta amount from the eval state that we deposited to the bid
          eval_state.sub_balance( balance_id_type(), delta_amount );
      }

      current_bid->last_update = eval_state._current_state->now();
      current_bid->balance     += this->amount;

      eval_state._current_state->store_bid_record( this->bid_index, *current_bid );

      //auto check   = eval_state._current_state->get_bid_record( this->bid_index );
   } FC_CAPTURE_AND_RETHROW( (*this) ) }

   /**
    *  If the amount is negative then it will withdraw/cancel the bid assuming
    *  it is signed by the owner and there is sufficient funds.
    *
    *  If the amount is positive then it will add funds to the bid.
    */
   void ask_operation::evaluate( transaction_evaluation_state& eval_state )
   { try {
      if( this->ask_index.order_price == price() )
         FC_CAPTURE_AND_THROW( zero_price, (ask_index.order_price) );

      auto owner = this->ask_index.owner;
      if( !eval_state.check_signature( owner ) )
         FC_CAPTURE_AND_THROW( missing_signature, (ask_index.owner) );

      asset delta_amount  = this->get_amount();

      eval_state.validate_asset( delta_amount );

      auto current_ask   = eval_state._current_state->get_ask_record( this->ask_index );


      if( this->amount == 0 ) FC_CAPTURE_AND_THROW( zero_amount );
      if( this->amount <  0 ) // withdraw
      {
          if( NOT current_ask )
             FC_CAPTURE_AND_THROW( unknown_market_order, (ask_index) );

          if( llabs(this->amount) > current_ask->balance )
             FC_CAPTURE_AND_THROW( insufficient_funds, (amount)(current_ask->balance) );

          // add the delta amount to the eval state that we withdrew from the ask
          eval_state.add_balance( -delta_amount );
      }
      else // this->amount > 0 - deposit
      {
          if( NOT current_ask )  // then initialize to 0
            current_ask = order_record();
          // sub the delta amount from the eval state that we deposited to the ask
          eval_state.sub_balance( balance_id_type(), delta_amount );
      }

      current_ask->last_update = eval_state._current_state->now();
      current_ask->balance     += this->amount;
      FC_ASSERT( current_ask->balance >= 0, "", ("current_ask",current_ask)  );

      eval_state._current_state->store_ask_record( this->ask_index, *current_ask );
   } FC_CAPTURE_AND_RETHROW( (*this) ) }

   void short_operation::evaluate( transaction_evaluation_state& eval_state )
   {
      auto owner = this->short_index.owner;
      FC_ASSERT( short_index.order_price.ratio < fc::uint128( 10, 0 ), "Interest rate must be less than 1000% APR" );

      asset delta_amount  = this->get_amount();

      eval_state.validate_asset( delta_amount );
      auto  asset_to_short = eval_state._current_state->get_asset_record( short_index.order_price.quote_asset_id );
      FC_ASSERT( asset_to_short.valid() );
      FC_ASSERT( asset_to_short->is_market_issued(), "${symbol} is not a market issued asset", ("symbol",asset_to_short->symbol) );

      auto current_short   = eval_state._current_state->get_short_record( this->short_index );
      //if( current_short ) wdump( (current_short) );

      if( this->amount == 0 ) FC_CAPTURE_AND_THROW( zero_amount );
      if( this->amount <  0 ) // withdraw
      {
          if( !eval_state.check_signature( owner ) )
             FC_CAPTURE_AND_THROW( missing_signature, (short_index.owner) );

          if( NOT current_short )
             FC_CAPTURE_AND_THROW( unknown_market_order, (short_index) );

          if( llabs(this->amount) > current_short->balance )
             FC_CAPTURE_AND_THROW( insufficient_funds, (amount)(current_short->balance) );

          // add the delta amount to the eval state that we withdrew from the short
          eval_state.add_balance( -delta_amount );
      }
      else // this->amount > 0 - deposit
      {
          FC_ASSERT( this->amount >=  0 ); // 100 XTS min short order
          if( NOT current_short )  // then initialize to 0
            current_short = order_record();
          // sub the delta amount from the eval state that we deposited to the short
          eval_state.sub_balance( balance_id_type(), delta_amount );
      }
      current_short->short_price_limit = this->short_price_limit;
      current_short->last_update = eval_state._current_state->now();
      current_short->balance     += this->amount;
      FC_ASSERT( current_short->balance >= 0 );

      eval_state._current_state->store_short_record( this->short_index, *current_short );
   }

   /**
     pay off part of the USD balance, if balance goes to 0 then close out
     the position and transfer collateral to proper place.
     update the call price (remove old value, add new value)
   */
   void cover_operation::evaluate( transaction_evaluation_state& eval_state )
   {
      if( this->cover_index.order_price == price() )
         FC_CAPTURE_AND_THROW( zero_price, (cover_index.order_price) );

      if( this->amount == 0 && !this->new_cover_price )
         FC_CAPTURE_AND_THROW( zero_amount );

      if( this->amount < 0 )
         FC_CAPTURE_AND_THROW( negative_deposit );

      asset delta_amount  = this->get_amount();

      if( !eval_state.check_signature( cover_index.owner ) )
         FC_CAPTURE_AND_THROW( missing_signature, (cover_index.owner) );


      // subtract this from the transaction
      eval_state.sub_balance( address(), delta_amount );

      auto current_cover   = eval_state._current_state->get_collateral_record( this->cover_index );
      if( NOT current_cover )
         FC_CAPTURE_AND_THROW( unknown_market_order, (cover_index) );

      auto  asset_to_cover = eval_state._current_state->get_asset_record( cover_index.order_price.quote_asset_id );
      FC_ASSERT( asset_to_cover.valid() );

      // calculate interest due on delta_amount
      asset interest_due = delta_amount * current_cover->interest_rate;
      const auto start_time = current_cover->expiration - fc::seconds( BTS_BLOCKCHAIN_MAX_SHORT_PERIOD_SEC );
      auto elapsed_sec = ( eval_state._current_state->now() - start_time ).to_seconds();

      if( elapsed_sec < 0 ) elapsed_sec = 0;

      fc::uint128 due = interest_due.amount;
      due *= elapsed_sec;
      due /= (365 * 24 * 60 * 60); // seconds per year

      asset amount_paid = delta_amount;
      amount_paid.amount -= due.to_uint64();

      // subtract interest due from amount paid to cover, add it to amount paid to fees

      asset_to_cover->current_share_supply -= amount_paid.amount; //delta_amount.amount;
      asset_to_cover->collected_fees += due.to_uint64();
      eval_state._current_state->store_asset_record( *asset_to_cover );

      current_cover->payoff_balance -= delta_amount.amount;
      // changing the payoff balance changes the call price... so we need to remove the old record
      // and insert a new one.
      eval_state._current_state->store_collateral_record( this->cover_index, collateral_record() );

      if( current_cover->payoff_balance > 0 )
      {
         auto new_call_price = asset( current_cover->payoff_balance, delta_amount.asset_id) /
                               asset( (current_cover->collateral_balance*2)/3, cover_index.order_price.base_asset_id );

         if( this->new_cover_price && (*this->new_cover_price > new_call_price) )
            eval_state._current_state->store_collateral_record( market_index_key( *this->new_cover_price, this->cover_index.owner ),
                                                                *current_cover );
         else
            eval_state._current_state->store_collateral_record( market_index_key( new_call_price, this->cover_index.owner ),
                                                                *current_cover );
      }
      else // withdraw the collateral to the transaction to be deposited at owners discretion / cover fees
      {
         eval_state.add_balance( asset( current_cover->collateral_balance, cover_index.order_price.base_asset_id ) );
      }
   }

   void add_collateral_operation::evaluate( transaction_evaluation_state& eval_state )
   {
      if( this->cover_index.order_price == price() )
         FC_CAPTURE_AND_THROW( zero_price, (cover_index.order_price) );

      if( this->amount == 0 )
         FC_CAPTURE_AND_THROW( zero_amount );

      if( this->amount < 0 )
         FC_CAPTURE_AND_THROW( negative_deposit );

      asset delta_amount  = this->get_amount();
      eval_state.sub_balance( address(), delta_amount );

      // update collateral and call price
      auto current_cover   = eval_state._current_state->get_collateral_record( this->cover_index );
      if( NOT current_cover )
         FC_CAPTURE_AND_THROW( unknown_market_order, (cover_index) );

      current_cover->collateral_balance += delta_amount.amount;

      // changing the payoff balance changes the call price... so we need to remove the old record
      // and insert a new one.
      eval_state._current_state->store_collateral_record( this->cover_index, collateral_record() );

      auto new_call_price = asset( current_cover->payoff_balance, cover_index.order_price.quote_asset_id ) /
                            asset( (current_cover->collateral_balance*2)/3, cover_index.order_price.base_asset_id );

      eval_state._current_state->store_collateral_record( market_index_key( new_call_price, this->cover_index.owner),
                                                          *current_cover );
   }

   void remove_collateral_operation::evaluate( transaction_evaluation_state& eval_state )
   {
      // Should this even be allowed?
      FC_ASSERT( !"Not implemented!" );
   }
    
    balance_id_type  buy_chips_operation::balance_id()const
    {
        withdraw_condition condition(withdraw_with_signature( this->owner ), this->amount.asset_id);
        return condition.get_address();
    }
    
    void buy_chips_operation::evaluate( transaction_evaluation_state& eval_state )
    { try {
        if ( this->amount.amount == 0) {
            FC_CAPTURE_AND_THROW( zero_amount );
        }
        
        if( this->amount.amount < 0 )
            FC_CAPTURE_AND_THROW( negative_deposit );
        
        auto  asset_to_buy = eval_state._current_state->get_asset_record( this->amount.asset_id );

        FC_ASSERT( asset_to_buy.valid() );
        FC_ASSERT( asset_to_buy->is_chip_asset(), "${symbol} is not a chip asset", ("symbol",asset_to_buy->symbol) );
        
        double price = (asset_to_buy->current_collateral * 1.0) / asset_to_buy->current_share_supply;
        share_type collateral_to_add = this->amount.amount * price;
        
        asset cost_shares( collateral_to_add, 0);
        
        eval_state.sub_balance( this->owner, cost_shares);
        
        // deposit amount of chips to the owner. and update the asset record's current collateral
        
        withdraw_condition condition( withdraw_with_signature( this->owner ), this->amount.asset_id);
        
        auto chips_balance_id = condition.get_address();
        auto cur_record = eval_state._current_state->get_balance_record( chips_balance_id );
        if( !cur_record )
        {
            cur_record = balance_record( condition );
        }
        cur_record->last_update   = eval_state._current_state->now();
        cur_record->balance       += this->amount.amount;
        // because this is system created from vitual, so no need to sub_balance
        eval_state._current_state->store_balance_record( *cur_record );
        
        asset_to_buy->current_collateral += collateral_to_add;
        asset_to_buy->current_share_supply += this->amount.amount;
        // TODO: Dice, do we need to update the asset 0's current supply, no. Because it is just become the collateral of chips.
        
        eval_state._current_state->store_asset_record( *asset_to_buy );
    } FC_CAPTURE_AND_RETHROW( (*this) ) }
    
    balance_id_type  sell_chips_operation::balance_id()const
    {
        withdraw_condition condition(withdraw_with_signature( this->owner ), 0);
        return condition.get_address();
    }
    
    void sell_chips_operation::evaluate( transaction_evaluation_state& eval_state )
    { try {
        if ( this->amount.amount == 0) {
            FC_CAPTURE_AND_THROW( zero_amount );
        }
        
        if( this->amount.amount < 0 )
            FC_CAPTURE_AND_THROW( negative_deposit );
        
        auto  asset_to_sell = eval_state._current_state->get_asset_record( this->amount.asset_id );
        
        FC_ASSERT( asset_to_sell.valid() );
        FC_ASSERT( asset_to_sell->is_chip_asset(), "${symbol} is not a chip asset", ("symbol",asset_to_sell->symbol) );
        
        double price = (asset_to_sell->current_collateral * 1.0) / asset_to_sell->current_share_supply;
        share_type collateral_to_remove = this->amount.amount * price;
        
        eval_state.sub_balance( this->owner, this->amount);
        
        asset shares_to_seller( collateral_to_remove, 0);
        
        // deposit amount of chips to the owner. and update the asset record's current collateral
        
        withdraw_condition condition( withdraw_with_signature( this->owner ), 0);
        
        auto lto_balance_id = condition.get_address();
        auto cur_record = eval_state._current_state->get_balance_record( lto_balance_id );
        if( !cur_record )
        {
            cur_record = balance_record( condition );
        }
        cur_record->last_update   = eval_state._current_state->now();
        cur_record->balance       += shares_to_seller.amount;
        // because this is system created from vitual, so no need to sub_balance
        eval_state._current_state->store_balance_record( *cur_record );
        
        FC_ASSERT( collateral_to_remove <= asset_to_sell->current_collateral,
                  "The collateral to remove is larger the the asset's current collateral ${symbol}", ("symbol",asset_to_sell->symbol)("collateral_remove", collateral_to_remove)("current_collateral", asset_to_sell->current_collateral));
        FC_ASSERT( this->amount.amount <= asset_to_sell->current_share_supply,
                  "The amount to sell is larger than the asset's current supply ${symbol}", ("symbol",asset_to_sell->symbol)
                  ("sell_amount", this->amount.amount)("current_supply", asset_to_sell->current_share_supply));
        
        asset_to_sell->current_collateral -= collateral_to_remove;
        asset_to_sell->current_share_supply -= this->amount.amount;
        // TODO: Dice, do we need to update the asset 0's current supply, no. Because it is just become the collateral of chips.
        
        eval_state._current_state->store_asset_record( *asset_to_sell );
    } FC_CAPTURE_AND_RETHROW( (*this) ) }

} } // bts::blockchain
