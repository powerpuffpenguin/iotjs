console.log(new Date())
var at = Date.now()
var hex = require("iotjs/encoding/hex")
var http = require("iotjs/net/http")
http.request("https://srv-ms2022.cdnewstar.cn/view/zh-Hant/").then(function (resp) {
    console.log("resp:", resp.code, resp.header)
    if (resp.body) {
        var body = new TextDecoder().decode(resp.body)
        console.log(body)
    }

}).catch(function (e) {
    console.log("err:", e.toString())
})

var used = (Date.now() - at) / 1000
console.log("used", used + "s")
