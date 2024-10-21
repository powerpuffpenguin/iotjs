if (iotjs.arch === "csky") {
    iotjs.nameserver("192.168.251.1:53")
} else {
    iotjs.nameserver("114.114.114.114")
}
var net = require("iotjs/net")
// var WebsocketConn = net.WebsocketConn
// try {
//     WebsocketConn.connect(
//         "wss://xsd-kb2022-dev.cdnewstar.cn:8001/api/v1/player/listen?player_token=Bearer%20CEUSFDUxMzM1Njc4OTAxMjM0NTY3OTY3GhQxMjM0NTY3ODkwMTIzNDU2Nzg3OCCorJanBg.pEkDMZDs9P_6fVjEf-zJ2A",
//         {
//             timeout: 1000,
//             insecure: true,
//         }, function (c, e) {
//             if (c) {
//                 console.log("connect success", c)
//                 c.send(JSON.stringify({ what: 123 }))
//                 // c.close()
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
//                     // setInterval(function () {
//                     //     console.log(new Date())
//                     // }, 1000)
//                     // setTimeout(function () {

//                     // }, 1000);
//                 }

//                 c.onMessage = function (data) {
//                     console.log(data)
//                 }
//             } else {
//                 console.log("connect err:", e.toString())
//             }
//         })
// } catch (e) {
//     console.log('-------', e.toString())
// }
var hex = require("iotjs/encoding/hex")
var sha256 = require("iotjs/crypto/sha256")
var md5 = require("iotjs/crypto/md5")
var cipher = require("iotjs/crypto/cipher")
var opts = {
    key: sha256.sum("12345678901"),
    iv: md5.sum("this is iv"),
}
var encoder = new cipher.OFBEncryptor(opts)
var decryptor = new cipher.OFBDecryptor(opts)

var s = "123草7890123草1"
var b = new TextEncoder().encode(s)
var enc = new Uint8Array(b.length)
encoder.encrypt(enc, b)
console.log(s, hex.encodeToString(s))
console.log(hex.encodeToString(enc))

decryptor.decrypt(enc, enc)
console.log(new TextDecoder().decode(enc))

