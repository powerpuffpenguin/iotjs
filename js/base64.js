var base64 = require("iotjs/encoding/base64")
var encs = [
    base64.std,
    base64.rawSTD,
    base64.std,
    base64.rawSTD,
]
var s0 = "cerberus is an idea, 中文測試"
for (var i = 0; i < encs.length; i++) {
    var enc = encs[i]
    var b = new TextEncoder().encode(s0)
    var l1 = enc.encodedLen(b.byteLength)
    console.log("encodedLen(" + b.byteLength + ")", l1)
    var s1 = enc.encodeToString(s0)
    console.log("encodeToString", s1, s1.length)

    var l0 = enc.decodedLen(l1)
    console.log("decodedLen(" + l1 + ")", l0)
    var s = enc.decodeToString(s1)
    console.log("decodeToString", s, new TextEncoder().encode(s).byteLength, "\n")
}