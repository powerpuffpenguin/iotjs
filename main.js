var hex = require("iotjs/encoding/hex")
var net = require("iotjs/net")

net.resolveIP("ip4", "www.baidu.com").then(function (v) {
    if (Array.isArray(v)) {
        for (var i = 0; i < v.length; i++) {
            const ip = v[i];
            console.log(i, ip.string())
        }
    }

}, function (e) {
    console.log("err 3", e.toString())
}).then(function () {
    console.log("------")
    net.resolveIP("ip6", "ip6-allnodes").then(function (v) {
        if (Array.isArray(v)) {
            for (var i = 0; i < v.length; i++) {
                const ip = v[i];
                console.log(i, ip.string())
            }
        }
    }, function (e) {
        console.log("err 2", e.string())
    })
}).then(function () {
    console.log("******")
    return net.resolveIP('ip', "localhost").then(function (v) {
        if (Array.isArray(v)) {
            for (var i = 0; i < v.length; i++) {
                const ip = v[i];
                console.log(i, ip.string())
            }
        }
    }, function (e) {
        console.log(e.toString())
    })
}, function (e) {
    console.log("err 1", e.toString())
})
