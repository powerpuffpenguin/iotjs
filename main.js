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
if (iotjs.arch === "csky") {
    iotjs.nameserver("192.168.251.1:53")
}
var net = require("iotjs/net")
var TCPConn = net.TCPConn
try {
    TCPConn.connect("192.168.251.50", 12233, {
        timeout: 1000,
        tls: true,
        insecure: true,
    }).then(function (c) {
        c.debug = true
        // c.onWritable = function () {
        //     console.log("onWritable")
        // }
        // c.onReadable = function () {
        //     console.log("onReadable")
        // }
        // c.onClose = function () {
        //     console.log("onClose")
        // }
        var x = 0
        c.onMessage = function (data) {
            x++
        }
        setInterval(function () {
            console.log(x)
            x = 0
        }, 1000);
        try {
            console.log("connect success", c)
            // var b = new ArrayBuffer(1024 * 1024)
            // for (var i = 0; i < 2; i++) {
            //     if (!c.tryWrite(b)) {
            //         console.log("no")

            //     }
            // }
        } catch (e) {
            console.log("tcp err:", e.toString())
            c.close()
        }
    }).catch(function (e) {
        console.log("connect err:", e.toString())
    })
} catch (e) {
    console.log('-------', e)
}

