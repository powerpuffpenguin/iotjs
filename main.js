// if ( iotjs.arch === "csky") {
//     iotjs.nameserver("192.168.251.1:53")
// }
// var net = require("iotjs/net")
// var WebsocketConn = net.WebsocketConn
// try {
//     WebsocketConn.connect(
//         "wss://192.168.251.50:9443/abc",
//         {
//             timeout: 1000,
//             insecure: true,
//         }, function (c, e) {
//             if (c) {
//                 console.log("connect success", c)
//                 c.debug = true
//                 c.onError = function (e) {
//                     if (e instanceof Error) {
//                         console.log("js onError", e.toString())
//                     } else {
//                         console.log("js onError", e)
//                     }
//                 }
//                 c.onClose = function () {
//                     console.log("js onClose")
//                 }
//                 // c.onReadable = function () {
//                 //     console.log("js onReadable")
//                 // }
//                 var i = 0
//                 c.onMessage = function (data) {
//                     if (typeof data == "string") {
//                         console.log("js onMessage string", data.length)
//                         console.log(data)
//                     } else {
//                         console.log("js onMessage Uint8Array", data.length)
//                         console.log(data)
//                     }
//                     if (i < 100) {
//                         c.trySend("this is message " + i)
//                         i++
//                     } else {
//                         c.close()
//                     }
//                 }
//             } else {
//                 console.log("connect err:", e.toString())
//             }
//         })
// } catch (e) {
//     console.log('-------', e.toString())
// }
console.log('2'.padStart(5, '01'))
console.log('2'.padStart(4, '01'))
console.log('2'.padEnd(5, '01'))
console.log('2'.padEnd(4, '01'))