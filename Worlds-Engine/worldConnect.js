// This is responcible to connected to the worlds client
// Is has various commands for moving items into the game etc

var net = require('net');
var fs = require('fs')

var world = {
    wor_sock: null,
    enter: function() {
        
        if (self.wor_sock == null) { 
            console.log('Conecting');
            IP = document.getElementById("world_client_ip").value;
            Port = document.getElementById("world_client_port").value; 
            self.wor_sock = new net.Socket();
            self.wor_sock.connect(Port, IP);
            console.log('Connecting ' + IP + Port);
        }
        
        // Time to get all your shit into the world
        files = fs.readdirSync('items')
        for(var i = 0 ; i < files.length ; i++) {
            if(files[i] == 'placeholder') {
                // delete files[i]
                    files.splice(i)
            }
        }
        
        // Dump all the items over to the server meow.
        for(i = 0 ; i < files.length ; i++) {
            item = fs.readFileSync('items/' + files[i]);
            console.log('Sending' + item);
            self.wor_sock.write(item);
        }
    }
}
