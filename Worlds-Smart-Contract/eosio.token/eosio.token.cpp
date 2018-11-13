/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "eosio.token.hpp"

namespace eosio{


void WSC::createitem( name         owner,        // Creator of this item.
                      string       item_name,    // Name of the item.
                      string       item_class,   // Class of the item.
                      asset        stake         // How much WOR to stake into the item.
                    )
{

  require_auth( owner );
  require_recipient( owner );
  
  auto sym = stake.symbol;

  stats statstable( _self, sym.raw() );
  const auto& st = statstable.get( sym.raw() );
  
  eosio_assert( stake.is_valid(), "invalid quantity" );                           
  eosio_assert( stake.amount > 0, "must transfer positive quantity" );
  eosio_assert( stake.symbol == st.supply.symbol, "symbol precision mismatch" );
  
  capi_checksum256 calc_hash;
  
  calc_hash = hashItemCreate(owner, item_name, item_class, stake);
  
  itemProof_table itemProof(_self, owner);

  // Place the hash onchain   
  itemProof.emplace(owner, [&](auto& p) {
    p.itemHash = calc_hash;
  });
  
  sub_balance( owner, stake );
  
};

void WSC::liquidateitem( name                owner,    // Who's the owner.
                         item                tx_item,  // Actual item package.
                         capi_checksum256    hash      // What's the hash of the item.
                       )
{
/*
  require_auth( owner );
  require_recipient( owner );

  capi_checksum256 calc_hash;
  sha256((char*) &tx_item.ItemName, sizeof(tx_item), &calc_hash);
  eosio_assert(calc_hash == hash, "Hash does not match"); // Ensure the hash matches the item hash.
  
  itemProof_table itemProof(_self, owner);
  // auto chainHash = itemProof.get(*(uint64_t*)&hash); // Check to see if the item is onchain.
  auto itr = itemProof.find(*(uint64_t*)&calc_hash);
  auto chainHash = itemProof.get(*(uint64_t*)&calc_hash); 

  eosio_assert(chainHash.itemHash == hash, "Hash on chain does not match!");
  itemProof.erase(itr); // Remove the hash from the table.

  add_balance( owner, tx_item.Stake, owner );
*/
}

void WSC::transferitem( name   from,     // Who's sending the item.
                        name   to,       // Who's getting the item.
                        item           tx_item,  // Actual item package.
                        capi_checksum256    hash      // What's the hash of the item.
                      )
{
/*
  require_auth( from );
  require_recipient( from );
  require_recipient( to );

  capi_checksum256 calc_hash;
  sha256((char*) &tx_item.ItemName, sizeof(tx_item), &calc_hash);
  
  // Ensure the hash matches the item hash.
  eosio_assert(calc_hash == hash, "Hash does not match"); 
  
  itemProof_table itemProofFrom(_self, from);
  
  // Check to see if the item is onchain.
  auto itr = itemProofFrom.find(*(uint64_t*)&calc_hash);
  auto chainHash = itemProofFrom.get(*(uint64_t*)&calc_hash);

  eosio_assert(chainHash.itemHash == hash, "Hash on chain does not match!");
  itemProofFrom.erase(itr); // Remove the hash from the table.

  itemProof_table itemProofTo(_self, to);
  calc_hash = hashItemTransfer(to, tx_item);

  itemProofTo.emplace(from, [&](auto& p) {
    p.itemHash = calc_hash;
  });
*/
}


void WSC::createwor( name   issuer,
                    asset  maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void WSC::issuewor( name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
      SEND_INLINE_ACTION( *this, transferwor, { {st.issuer, "active"_n} },
                          { st.issuer, to, quantity, memo }
      );
    }
}

void WSC::retirewor( asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must retire positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

void WSC::transferwor( name    from,
                      name    to,
                      asset   quantity,
                      string  memo )
{
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.code();
    stats statstable( _self, sym.raw() );
    const auto& st = statstable.get( sym.raw() );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto payer = has_auth( to ) ? to : from;

    sub_balance( from, quantity );
    add_balance( to, quantity, payer );
}

void WSC::sub_balance( name owner, asset value ) {
   accounts from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
}

void WSC::add_balance( name owner, asset value, name ram_payer )
{
   accounts to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void WSC::openwor( name owner, const symbol& symbol, name ram_payer )
{
   require_auth( ram_payer );

   auto sym_code_raw = symbol.code().raw();

   stats statstable( _self, sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
   eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch" );

   accounts acnts( _self, owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }
}

void WSC::closewor( name owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( _self, owner.value );
   auto it = acnts.find( symbol.code().raw() );
   eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

capi_checksum256 WSC::hashItemTransfer(name NewOwner, WSC::item item)
{
  capi_checksum256 calc_hash;

  item.Owner = NewOwner;
  sha256((char*) &item.ItemName, sizeof(item), &calc_hash);
  
  // Make this hash match!
  print("ItemName: ", item.ItemName, "\n");
  print("ItemClass: ", item.ItemClass, "\n");
  print("ItemOwner: ", item.Owner, "\n");
  print("PreviousOwner: ", item.PreviousOwner, "\n");
  print("OriginWorld: ", item.OriginWorld, "\n");
  print("GenesisTime: ", item.GenesisTime, "\n");
  print("TXtime: ", item.TXtime, "\n");
  print("Stake: ", item.Stake, "\n");
  
  return(calc_hash); 

}


capi_checksum256 WSC::hashItemCreate(name owner, string item_name, string item_class, asset stake)
{

  capi_checksum256 calc_hash;
  WSC::item item;

  // Fill the structure. 
  item.ItemName = item_name;
  item.ItemClass = item_class;
  item.Owner = owner;
  // item.PreviousOwner = 0x00;
  item.OriginWorld = owner;
  item.GenesisTime = now();
  item.TXtime = 0x00;
  item.Stake = stake;
  
  // Make this hash match!
  print("ItemName: ", item.ItemName, "\n");
  print("ItemClass: ", item.ItemClass, "\n");
  print("ItemOwner: ", item.Owner, "\n");
  print("PreviousOwner: ", item.PreviousOwner, "\n");
  print("OriginWorld: ", item.OriginWorld, "\n");
  print("GenesisTime: ", item.GenesisTime, "\n");
  print("TXtime: ", item.TXtime, "\n");
  print("Stake: ", item.Stake, "\n");

  sha256((char*) &item.ItemName, sizeof(item), &calc_hash);
  
  // for (int i = 0 ; i < 32 ; i++)
  print("Sha256: ");
  printhex((const void*)&calc_hash, 32);
  
  return(calc_hash);


}
}