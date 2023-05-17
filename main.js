var fs = require("iotjs/fs")
console.log('fs', fs)
console.log("stat")
var stat = fs.stat("./main.js")
stat.then(function (info) {
    console.log("info:", info, new Date())
    // x
}, function (e) {
    console.log("err:", e.toString(), e instanceof Error)
})
console.log("main end")
