console.log(new Date())
var at = Date.now()
var hex = require("iotjs/encoding/hex")
var http = require("iotjs/net/http")

console.log(http.http())

var used = (Date.now() - at) / 1000
console.log("used", used + "s")
