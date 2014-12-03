#include <bts/blockchain/market_records.hpp>
#include <fc/exception/exception.hpp>
#include <fc/reflect/variant.hpp>
#include <sstream>
#include <boost/algorithm/string.hpp>

namespace bts { namespace blockchain {

order_id_type market_order::get_id()const
{
    std::stringstream id_ss;
    id_ss << string( type )
          << string( market_index.order_price )
          << string( market_index.owner );
    return fc::ripemd160::hash( id_ss.str() );
}

string market_order::get_small_id()const
{
    string type_prefix = string( type );
    type_prefix = type_prefix.substr( 0, type_prefix.find( "_" ) );
    boost::to_upper( type_prefix );
    return type_prefix + "-" + string( get_id() ).substr( 0, 8 );
}

asset market_order::get_balance()const
{
  asset_id_type asset_id;
  switch( order_type_enum( type ) )
  {
     case relative_bid_order:
     case bid_order:
        asset_id = market_index.order_price.quote_asset_id;
        break;
     case relative_ask_order:
     case ask_order:
        asset_id = market_index.order_price.base_asset_id;
        break;
     case null_order:
        FC_ASSERT( !"Null Order" );
  }
  return asset( state.balance, asset_id );
}

price market_order::get_price( const price& relative )const
{ try {
   switch( order_type_enum(type) )
   {
      case relative_bid_order:
      case relative_ask_order:
         if( relative != price() )
            return market_index.order_price + relative;
         else
            return market_index.order_price;
      case bid_order:
      case ask_order:
      case null_order:
        FC_ASSERT( !"Null Order" );
   }
   FC_ASSERT( !"Should not reach this line" );
} FC_CAPTURE_AND_RETHROW( (*this)(relative) ) }

asset market_order::get_quantity( const price& relative )const
{
  switch( order_type_enum( type ) )
  {
     case relative_bid_order:
     case bid_order:
     { // balance is in USD  divide by price
        return get_balance() * get_price(relative);
     }
     case relative_ask_order:
     case ask_order:
     { // balance is in USD  divide by price
        return get_balance();
     }
     default:
        FC_ASSERT( false, "Not Implemented" );
  }
  // NEVER GET HERE.....
  //return get_balance() * get_price();
}
asset market_order::get_quote_quantity( const price& relative )const
{
  switch( order_type_enum( type ) )
  {
     case relative_bid_order:
     case bid_order:
     { // balance is in USD  divide by price
        return get_balance();
     }
     case relative_ask_order:
     case ask_order:
     { // balance is in USD  divide by price
        return get_balance() * get_price(relative);
     }
     default:
        FC_ASSERT( false, "Not Implemented" );
  }
  // NEVER GET HERE.....
 // return get_balance() * get_price();
}

} } // bts::blockchain
