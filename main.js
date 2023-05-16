// var fs = require("iotjs/fs")
// var stat = fs.stat("main.js")
// console.log(stat)
var x = setTimeout(function () {
    console.log("never")
}, 1000);
setTimeout(function () {
    console.log("ok")
    clearTimeout(x);

    var i = 0
    var y = setInterval(function () {
        console.log(i++)
    }, 100);
    setTimeout(function () {
        console.log("close")
        clearInterval(y)
    }, 1000);
}, 100);
