const {app, BrowserWindow} = require('electron')
const actions = require('./actions')
const sock = require('./socket')
var fs = require('fs')

let mainWindow

/*
var nodeConsole = require('console');
var Myconsole = new nodeConsole.Console(process.stdout, process.stderr);

/* Replace console with special output
var console = {};
console.log = function(msg){
  Myconsole.log(msg)
};
*/

var eos
 
var account = {
  pubKey : null,
  privKey : null,
  name : null,
	selectedName : null,
	balance : null
}

// const element = <h1>Hello, {formatName(account.name)}!</h1>;

function connect() {
  account.privKey = document.getElementById("PrivateKey").value
  account.pubKey = document.getElementById("PublicKey").value
  Port = document.getElementById("Port").value
  EndPoint= document.getElementById("EndPoint").value
  ChainID = document.getElementById("ChainID").value
  
  console.log('Unlocking Wallet and connect to endpoint!');
  
  sock.open(Port) // Open socket
  eos = actions.connectEndpoint(ChainID, EndPoint, account.privKey)
  eos.getKeyAccounts(account.pubKey).then(addName)
  }

function addName(result){
	account.name = result.account_names 
	
	var x = document.getElementById("Chars");
	
	for(i = 0 ; i < account.name.length ; i++){
		var option = document.createElement("option");
		option.text = account.name[i] 
		x.add(option)
	}
}

function updateChar(){
	account.selectedName = document.getElementById("Chars").value
	eos.getCurrencyBalance('wsc.code', account.selectedName, 'WOR').then(function fun(result) {account.balance = result; updateHTML(result)})
}

function updateHTML(result){
	document.getElementById("Balance").innerHTML = "Balance: " + account.balance
}

function TransferWor(){
  to = document.getElementById("TransferTo").value
  amount = document.getElementById("TransferAmt").value
  memo = 'future shit'

  actions.TXwor(eos, account.selectedName, to, amount, memo) 
}

function createItem(){

  ItemName = document.getElementById("ItemName").value
  ItemClass = document.getElementById("ItemClass").value
  ItemStake = document.getElementById("ItemStake").value
  ItemNuance = document.getElementById("ItemNuance").value
  
  if(account.balance < ItemStake){
    console.log('Insufficant Funds!')
    return(0)
  }

  console.log('Creating Item')
  actions.createItem(eos, account.selectedName, ItemName, ItemClass, ItemStake, ItemNuance) 
}

function deleteItem(){

  ItemHash = document.getElementById("ItemHash").value
  
  console.log('Deleting item' + ItemHash)
  actions.deleteItem(eos, account.selectedName, ItemHash) 
}

function loadItems(){
  files = fs.readdirSync('items')
  
  for(var i = 0 ; i < files.length ; i++){
    if(files[i] == 'placeholder'){
      // delete files[i]
      files.splice(i)
    }
  }

  var x = document.getElementById("Items_Select");
  for(i = 0 ; i < files.length ; i++){
		var option = document.createElement("option");
		option.text = files[i] 
		x.add(option)
	}
  
}

function createWindow () {
  
  mainWindow = new BrowserWindow({width: 400, height: 800})

  mainWindow.loadFile('index.html')

  mainWindow.webContents.openDevTools()

  mainWindow.on('closed', function () {
    mainWindow = null
  })
}

app.on('ready', createWindow)

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', function () {
  if (mainWindow === null) {
    createWindow()
  }
})

