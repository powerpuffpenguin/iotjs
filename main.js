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
var WebsocketConn = net.WebsocketConn
try {
    WebsocketConn.connect(
        "wss://192.168.251.50:9443/abc",
        {
            timeout: 1000,
            insecure: true,
        }).then(function (c) {
            c.debug = true
            c.onError = function (e) {
                if (e instanceof Error) {
                    console.log("js onError", e.toString())
                } else {
                    console.log("js onError", e)
                }
            }
            // c.onReadable = function () {
            //     console.log("js onReadable")
            // }
            var i = 0
            c.onMessage = function (data) {
                if (typeof data == "string") {
                    console.log("js onMessage string", data.length)
                    console.log(data)
                } else {
                    console.log("js onMessage Uint8Array", data.length)
                    console.log(data)
                }
                if (i < 10) {
                    c.trySend("this is message " + i)
                    i++
                } else {
                    c.close()
                }
            }
            c.onClose = function () {
                console.log("js onClose")
            }
            try {
                console.log("connect success", c)
            } catch (e) {
                console.log("ws err:", e.toString())
                c.close()
            }
        }).catch(function (e) {
            console.log("connect err:", e.toString())
        })
} catch (e) {
    console.log('-------', e.toString())
}

