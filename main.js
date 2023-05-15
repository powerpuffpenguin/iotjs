// var x = require("iotjs")
// console.log(JSON.stringify(x))
// var x = {
// }
// x.abc = undefined
// delete x["abc"]
// console.log(Object.hasOwnProperty.call(x,"abc"))
// for (const key in object) {
//     if (Object.hasOwnProperty.call(object, key)) {
//         const element = object[key];

//     }
// }
var x = [1, 2, 3]
x.forEach(function (v) {
    console.log(v)
})
var m = new Map()
m.set(1, 2)
console.log(m.get(1))
new Promise(function (resolve, reject) {
    setTimeout(function () {
        resolve(Date.now())
    }, 1000);
}).then(function (v) {
    console.log(v)
    return v + 1
}).then(function (v) {
    var s = "ok"
    s = Array.from(s)
    console.log(s)
})
// console.log('strings'.padStart(20, "-"))
// // require("./a")
// console.log("ok")
// x = require("iotjs")
// console.log(JSON.stringify(x))
// console.log(Promise)
// require("iotjs")
// console.log(1)
// var timer = setTimeout(function () {
//     console.log("test clearTimeout")
// }, 100);
// console.log('timer', timer, typeof timer)
// setTimeout(function () {
//     console.log(3)
//     clearTimeout(timer)
//     setTimeout(function () {
//         console.log("timer end")
//         var i = 0
//         var interval = setInterval(function () {
//             i++
//             console.log("interval", i)
//         }, 100);
//         setTimeout(function () {
//             clearInterval(interval)
//         }, 1000);
//     }, 1000);
// }, 0);
// console.log(2)
