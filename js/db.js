var mtd = require("iotjs/mtd")
var last = Date.now()
try {
    var path = "/dev/mtd10"
    var f = new mtd.File(path)
    console.log(f.info())
    f.close()
    var db = new mtd.DB(path)
    try {
        db.setSync("ko", new TextEncoder().encode("12345"))
        var val = db.getSync("ko")
        console.log('-------read', new TextDecoder().decode(val))
    } finally {
        db.close()
    }
} catch (e) {
    console.log(e instanceof Error ? e.toString() : e)
}
console.log("used", Date.now() - last + "ms")