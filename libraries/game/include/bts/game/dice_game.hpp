#pragma once

#include <fc/io/enum_type.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <bts/blockchain/address.hpp>
#include <bts/blockchain/types.hpp>
#include <bts/blockchain/withdraw_types.hpp>
#include <bts/game/games.hpp>
#include <bts/blockchain/transaction_evaluation_state.hpp>

#include <bts/blockchain/chain_database.hpp>
#include <bts/wallet/wallet.hpp>
#include <bts/wallet/wallet_records.hpp>

/**
 *  The C keyword 'not' is NOT friendly on VC++ but we still want to use
 *  it for readability, so we will have the pre-processor convert it to the
 *  more traditional form.  The goal here is to make the understanding of
 *  the validation logic as english-like as possible.
 */
#define NOT !

namespace bts { namespace game {
    using namespace bts::blockchain;
    using namespace bts::wallet;
    
    struct dice_game
    {
        static const game_type_enum    type;
        
        dice_game():amount(0), odds(1){}
        
        dice_game( const bts::blockchain::address& owner, bts::blockchain::share_type amnt, uint32_t odds = 2, uint32_t g = 1 );
        
        bts::blockchain::address owner()const;
        
        /** owner is just the hash of the condition */
        bts::blockchain::balance_id_type                balance_id()const;
        
        bts::blockchain::share_type          amount;
        uint32_t            odds;
        uint32_t            guess;
        
        /** the condition that the funds may be withdrawn,
         *  this is only necessary if the address is new.
         */
        bts::blockchain::withdraw_condition  condition;
        
        void evaluate( transaction_evaluation_state& eval_state );
        
        static wallet_transaction_record play( chain_database_ptr blockchain, bts::wallet::wallet_ptr w, const variant& params, bool sign);
    };
    
    struct dice_input
    {
        dice_input() {}
        dice_input( std::string name, double a, uint32_t o, uint32_t g )
        :from_account_name(name), amount(a), odds(o), guess(g) {}
        
        std::string     from_account_name;
        double          amount;
        uint32_t        odds;
        uint32_t        guess;
    };
    
} } // bts::game

FC_REFLECT( bts::game::dice_game, (amount)(odds)(guess)(condition) )
FC_REFLECT( bts::game::dice_input, (from_account_name)(amount)(odds)(guess) )
