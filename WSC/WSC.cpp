/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "WSC.hpp"
#include <eosio/system.hpp>
#include <eosio/time.hpp>

namespace eosio{

void WSC::createitem( name         owner,        // Creator of this item.
                      string       item_name,    // Name of the item.
                      string       item_class,   // Class of the item.
                      string       nuance,       // Special unique ID.
                      asset        stake         // How much WOR to stake into the item.
                    )
{

  require_auth( owner );
  require_recipient( owner );
  
  WSC::item item;
  
  item.Owner = owner;
  item.OriginWorld = owner;
  item.ItemName = item_name;
  item.ItemClass = item_class;
  item.Nuance = nuance;
  item.Stake= stake;

  auto sym = stake.symbol;

  stats statstable( _self, sym.code().raw() );
  const auto& st = statstable.get( sym.code().raw() );
  
  check( stake.is_valid(), "invalid quantity" );                           
  check( stake.amount > 0, "must transfer positive quantity" );
  check( stake.symbol == st.supply.symbol, "symbol precision mismatch" );
  sub_balance( owner, stake ); // Subtract Balance.
  
  item.GenesisTime = current_time_point().sec_since_epoch() / 3600; // Convert to hours 
  checksum256 calc_hash = hashItem(item);
  
  itemProof_table itemProof(_self, owner.value);
  
  // Place the hash onchain   
  itemProof.emplace(owner, [&](auto& p) {
    p.itemHash = calc_hash;
  });
};

void WSC::liquiditem(    item                tx_item // Who's the owner.
                    )
{
  require_auth( tx_item.Owner );
  require_recipient( tx_item.Owner );

  checksum256 calc_hash = hashItem(tx_item);

  itemProof_table itemProof(_self, tx_item.Owner.value);

  auto chainHash = itemProof.get(*(uint64_t*)&calc_hash); // Check to see if the item is onchain.
  auto itr = itemProof.find(*(uint64_t*)&calc_hash);

  if(!(memcmp((void *) &calc_hash, (void *) &chainHash.itemHash, 32))){
    itemProof.erase(itr); // Remove the hash from the table.
    add_balance(tx_item.Owner, tx_item.Stake, tx_item.Owner);
  }
}

void WSC::transferitem( name                to,       // Who's getting the item.
                        item                tx_item   // Actual item package.
                      )
{
  require_auth( tx_item.Owner );
  require_recipient( tx_item.Owner );
  require_recipient( to );

  checksum256 calc_hash = hashItem(tx_item);

  itemProof_table itemProofFrom(_self, tx_item.Owner.value);
  
  // Check to see if the item is onchain.
  auto chainHash = itemProofFrom.get(*(uint64_t*)&calc_hash);
  auto itr = itemProofFrom.find(*(uint64_t*)&calc_hash);
  
  if(!(memcmp((void *) &calc_hash, (void *) &chainHash.itemHash, 32))){
    itemProofFrom.erase(itr); // Remove the hash from the table.
    itemProof_table itemProofTo(_self, to.value);
    auto from = tx_item.Owner;
    tx_item.Owner = to;
    calc_hash = hashItem(tx_item);

    // 'from' is the gas payer.
    itemProofTo.emplace(from, [&](auto& p) {
      p.itemHash = calc_hash;
    });
  }
}

/*
  WARNING: This function deletes items without refunding WOR!
    It it mainly for refunding RAM if the user forgets the content of the items  
*/
void WSC::deleteitem( name                  owner,
                      checksum256           hash
                    ) 
{
  require_auth( owner );

  itemProof_table itemProof(_self, owner.value);
  auto itr = itemProof.find(*(uint64_t*)&hash);
  
  print("Deleting Item");
  itemProof.erase(itr); // GONE!
}

/* This function isn't working yet */
void WSC::tradeitem( item               tx_item, // What are you trading 
                     item               rx_item  // What are you traing for
                   )
{
  require_auth( tx_item.Owner );

  checksum256 tx_hash = sha256((char*) &tx_item.ItemName, sizeof(tx_item));
  checksum256 rx_hash = sha256((char*) &rx_item.ItemName, sizeof(rx_item));

  itemProof_table itemProof_tx(_self, tx_item.Owner.value);
  
  // Check to see if the item is onchain.
  auto chainHash = itemProof_tx.get(*(uint64_t*)&tx_hash);
  auto itr = itemProof_tx.find(*(uint64_t*)&tx_hash);

  assert_sha256((const char*) &tx_item.ItemName, sizeof(tx_item), chainHash.itemHash); // Ensure hash matches matches
  print("TX Item on chain and belongs to proper person");

  /*At this point the item the user wants to trade does in fact belog to the user*/
  print("Removing Item");
  itemProof_tx.erase(itr); // Remove the hash from the table.

  tradechannel_table channel(_self, rx_item.Owner.value); 
  
  auto itr_rx = channel.find(*(uint64_t*)&tx_hash); // Check to see if a trade channel isn't already open
  if(itr_rx == channel.end()){
    print("Found a trade channel open!");
    auto rx_channel = channel.get(*(uint64_t*)&tx_hash);
    
    if(rx_channel.tx_item == rx_hash){
      print("Trade Maches, Time to trade");
        
      checksum256 new_rx_hash, new_tx_hash;
      tx_item.Owner = rx_item.Owner;
      rx_item.Owner = tx_item.Owner;

      new_rx_hash = hashItem(rx_item);
      new_tx_hash = hashItem(tx_item);
  
      itemProof_table itemProof_rx(_self, rx_item.Owner.value);
      itemProof_table itemProof_tx(_self, tx_item.Owner.value);
  
      // Place the hash onchain   
      itemProof_rx.emplace(tx_item.Owner, [&](auto& p) {
        p.itemHash = new_rx_hash;
      });
      
      // The ram payer should really be the person receiving the item 
      itemProof_tx.emplace(tx_item.Owner, [&](auto& p) {
        p.itemHash = new_tx_hash;
      });
    }
    else{
      print("Trade does not match!");
    }
  }
  else{
    /* Now you need to create the trade channel and place it onchain */
  }

  /*
    -> Hash tx_item.
      -> Ensure it is on chain.
    -> Hash rx_item.
    -> Ensure there is not a trade channel open that has rx_item = hash(tx_item) && tx_item = hash(rx_item)
      -> If there is proceed to step X
    -> Then move the hash(tx_item) and hash(rx_item) into a trade channel
        -> This is a multi-index table that has two fields
          -> hash(tx_item)
          -> hash(rx_item)
    -> Delete the reference to tx_item on tx_item.Owner, it's now been moved into a trade channel.

    X-> Delete the trade channel and emplace tx_item on rx_item.Owners table. Then place rx_item on tx_item.Owners table!
     -> We've just made a trade :)

  */
}

void WSC::createwor( name   issuer,
                     asset  maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( maximum_supply.is_valid(), "invalid supply");
    check( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void WSC::issuewor( name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must issue positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

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
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must retire positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

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
    check( from != to, "cannot transfer to self" );
    require_auth( from );
    check( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.code();
    stats statstable( _self, sym.raw() );
    const auto& st = statstable.get( sym.raw() );

    require_recipient( from );
    require_recipient( to );

    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );
    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    auto payer = has_auth( to ) ? to : from;

    sub_balance( from, quantity );
    add_balance( to, quantity, payer );
}

void WSC::sub_balance( name owner, asset value ) {
   accounts from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

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
   check( st.supply.symbol == symbol, "symbol precision mismatch" );

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
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

checksum256 WSC::hashItem(WSC::item &item){

  int i = 0;
  string itemStake = item.Stake.to_string();
  string itemOwner = item.Owner.to_string();
  string itemOriginWorld = item.OriginWorld.to_string();
  string GenesisTime = std::to_string(item.GenesisTime);

  uint32_t size = item.ItemName.size() + item.ItemClass.size() + item.Nuance.size() + itemOwner.size() + itemOriginWorld.size() + GenesisTime.size() + itemStake.size();
  
  print(size, "\n");
  auto p = (char*) malloc(size);
  
  i = item.ItemName.copy(p, item.ItemName.size(), 0);
  print(i, "\n");

  i += item.ItemClass.copy(p+i, item.ItemClass.size(), 0);
  print(i, "\n");
  
  i += item.Nuance.copy(p+i, item.Nuance.size(), 0);
  print(i, "\n");
  
  i += itemOwner.copy(p+i, itemOwner.size(), 0);
  print(i, "\n");
  
  i += itemOriginWorld.copy(p+i, itemOriginWorld.size(), 0);
  print(i, "\n");

  i += GenesisTime.copy(p+i, GenesisTime.size(), 0);
  print(i, "\n");
  
  i += itemStake.copy(p+i, itemStake.size(), 0);
  print(i, "\n");
  
  checksum256 calc_hash = sha256(p, size);
  free(p);

  print("ItemName: ", std::move(item.ItemName), "\n");
  print("ItemClass: ", std::move(item.ItemClass), "\n");
  print("ItemNuance: ", std::move(item.Nuance), "\n");
  print("ItemOwner: ", std::move(itemOwner), "\n");
  print("OriginWorld: ", std::move(itemOriginWorld), "\n");
  print("GenesisTime: ", std::move(GenesisTime), "\n");
  print("Stake: ", std::move(itemStake), "\n");
 
  print("Sha256: ");
  calc_hash.print();
  return(calc_hash);

}
} // namespace eosio
