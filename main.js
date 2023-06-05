// var http = require("iotjs/net/http")
// var Websocket = http.Websocket
// try {
//     var at = Date.now()
//     Websocket.connect("wss://192.168.251.50:9443/ws", {
//         timeout: 5000,
//     }).then(function (ws) {
//         console.log("ws", ws)
//     }).catch(function (e) {
//         console.log("used", Date.now() - at)
//         console.log("err:", e.toString())
//         // setTimeout(function () {

//         // }, 1000);
//     })
// } catch (e) {
//     console.log('-------', e)
// }

var net = require("iotjs/net")
var TCPConn = net.TCPConn
try {
    TCPConn.connect("127.0.0.1", 9443, {
        timeout: 1000,
    }).then(function (c) {
        console.log(c)
        c.close()
        console.log(c)
    }).catch(function (e) {
        console.log("tcp err:", e.toString())
    })
} catch (e) {
    console.log('-------', e)
}

