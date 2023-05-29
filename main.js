console.log(new Date())
var at = Date.now()
var hex = require("iotjs/encoding/hex")
var http = require("iotjs/net/http")
http.request("https://www.baidu.com").then(function (resp) {
    console.log("resp", resp)
}).catch(function (e) {
    console.log("err", e.toString())
})
// http.test()

var used = (Date.now() - at) / 1000
console.log("used", used + "s")
