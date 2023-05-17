var fs = require("iotjs/fs")
console.log('fs', fs)
console.log("stat")
var stat = fs.stat("main.js")
stat.then(function (ok) {
    console.log("ok", ok)
}, function (e) {
    console.log("err", e.toString(), e instanceof Error)
})
console.log("main end")
