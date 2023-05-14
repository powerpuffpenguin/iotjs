// var x = require("iotjs")
// console.log(JSON.stringify(x))

// // require("./a")
// console.log("ok")
// x = require("iotjs")
// console.log(JSON.stringify(x))
// console.log(Promise)
console.log(1)
var timer = setTimeout(function () {
    console.log("test clearTimeout")
}, 100);
setTimeout(function () {
    console.log(3)
    // clearTimeout(timer)
    setTimeout(function () {
        console.log("timer end")
        var i = 0
        setInterval(function () {
            i++
            console.log("interval", i)
        }, 1000);
    }, 1000);
}, 0);
console.log(2)
