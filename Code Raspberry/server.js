const express = require('express');
const WebSocket = require('ws');
const path = require('path');

const app = express();

const wss = new WebSocket.Server({ port : 8015 });

wss.on('connection',(ws)=>{
    console.log('A new client Connected!');

    ws.on('message',(message)=>{
        console.log('received : ', message.toString());

        wss.clients.forEach((client)=>{
            if(client.readyState === WebSocket.OPEN) {
                client.send(message.toString());
            }
        });

    });

    ws.on('close',()=>{
        console.log('A client disconnected');
    });

});

app.use('/',express.static(__dirname));

app.get('/', (_req, res) =>{
    res.sendFile(path.join(__dirname,'main.htm'));
});

//Start the server
const port = 4500;
app.listen(port, () => {
    console.log(`Server is running on port ${port}`);
});
