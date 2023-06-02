var http = require("iotjs/net/http")
var Websocket = http.Websocket

Websocket.connect("wss://192.168.251.50:9443/ws", {
    timeout: 5000,
}).then(function (ws) {
    console.log("ws", ws)
}).catch(function (e) {
    console.log("err:", e.toString())
})