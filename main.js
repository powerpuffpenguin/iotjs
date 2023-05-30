console.log(new Date())
var at = Date.now()

console.log("---------------------")
var x = setTimeout(function () {
    console.log('ok')

}, 1000);
// clearTimeout(x)

var used = (Date.now() - at) / 1000
console.log("used", used + "s")
