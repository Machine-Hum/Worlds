# Worlds

<img src="Graphics/Header.png" alt="drawing" width="1000"/>

[YouTube Demo Here](https://www.youtube.com/watch?v=OrOZVr-j92A&t) <br>
[YouTube Explanation Here](https://www.youtube.com/watch?v=wfqsvlMFGHs&t)

Worlds is an open protocol designed to manage the economic and game theoretical components of a distributed MMO. This repository is focused on implementing the minimum dependencies required for developers to get up and running. For more detailed info please read the paper [here](https://github.com/Machine-Hum/Worlds/blob/master/Worlds-Whitepaper/whitepaper.pdf). This repository contains three main components.

* The Worlds Whitepaper
* The Worlds Engine
* The World Smart Contract

## Getting Started
First you want to source the enviornment.
```bash
source source_env.sh
```

Go ahead and create a wallet. 
```bash
wor_create_wallet
```

This will give you password, set up an init file to be called by other scripts.
Replace <PW>, <Pub> below with your password and pubkey. Keep in mind this is just
for development, do not do something like this with actual credentials.
```bash
echo "EOS_PW=<PW>" >> scripts/wor_init
echo "EOS_PUB=<Pub>" >> scripts/wor_init
echo "PJ_ROOT=$(pwd)" >> scripts/wor_init
source wor_init
```

Run the chain
```bash
wor_run_chain
```

Unlock your wallet and Create account. If cleos asked you to unlock your wallet,
use this unlock wallet script.
```bash
wor_unlock_wallet
wor_create_account
```

## Worlds Smart Contract 
The Worlds Smart Contact (WSC) is an EOS contract that is responsible for
managing player assets and WOR. WOR is a token that can be staked against items
and used as a means of currency. The contract is still under development and
subject to change!

### Building / Deploying
```bash
cd Worlds-Smart-Contract/  
make
wor_deploy_contract
```

### Dependencies
The WSC is build using eosio.cdt 1.3+
* [eosio](https://github.com/EOSIO/eos)
* [eosio.cdt](https://github.com/EOSIO/eosio.cdt)

## Scripting
Various simple scripts have been written to test the WSC. 

```bash
wor_create_tokens              # Create tokens
wor_issue_tokens               # Issue tokens
wor_get_token_balance turnip   # Get token balance of a player
wor_create_item                # Create items
wor_transfer_item              # Transfer Items
wor_get_table turnip           # Check the tables onchain.
wor_liquid_item                # Liquidate items back into WOR
```

## Worlds Client (Worlds.js)
The worlds client is wallet that enables players to view, create, trade and liquidate items, exp and WOR. The worlds client uses eosjs to call the WSC. It hosts a simple UDP server that will accept commands from the game client. It is also possible to manage game assets directly from the wallet. Please note the wallet is in beta!

Running the wallet
```bash
cd Worlds/Worlds-Engine
npm install
npm start
```

## Notes on the Project Ready Player One was the main inspiration for this
project, at the core of the novel is a centralisation issues over the control of
the Oasis. The idea of one company owning such a large game is an analogous to
one company owning the internet. When I started this project over six months ago
I wasn't even planning on using a blockchain. If you go back in the commit
history, you'll see that Worlds relied on an complex system of ledgers and
auditing. At the time there was no blockchain that could provide the rapid
finality required for my idea. In June 2018, the EOS main net launched and
satisfied the response requirements. This meant we could get higher security in
a less complex system developed in a fraction of the time.

Worlds uses a token, however there will be no ICO or crowd sale! 

## Contribution
If you wish to help with the project or are interested in developing a world, please reach out! Feedback welcome!
