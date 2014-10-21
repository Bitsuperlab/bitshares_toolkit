#include <bts/game/game_operations.hpp>
#include <bts/blockchain/chain_interface.hpp>
#include <bts/blockchain/exceptions.hpp>
#include <bts/game/game_factory.hpp>

namespace bts { namespace game {
    using namespace bts::blockchain;

    /**
     *  @note in this method we are using 'this->' to refer to member variables for
     *  clarity.
     */
    void game_operation::evaluate( transaction_evaluation_state& eval_state )
    { try {
        
        game_factory::instance().evaluate( eval_state, game );

    } FC_CAPTURE_AND_RETHROW( (*this) ) }

} } // bts::game
