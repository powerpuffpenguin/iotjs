var hex = require("iotjs/encoding/hex")
console.log('hex', hex)
var b = new ArrayBuffer(10)
console.log(b.byteLength)
var view = new DataView(b)
view.setUint32(0, 10)
var u0 = new Uint8Array(b)
console.log(hex.encodeLen(u0.length))
var s = hex.encodeToString(view)
console.log(s)
console.log(hex.encodeToString(new Float32Array([1, 2])))
// var stat = fs.stat("./main.js")
// stat.then(function (info) {
//     console.log("info:", info, new Date(), info.mode & fs.FileMode.regular ? true : false)
//     // x
// }, function (e) {
//     console.log("err:", e.toString(), e instanceof Error)
// })

