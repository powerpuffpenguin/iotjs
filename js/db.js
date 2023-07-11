var mtd = require("iotjs/mtd")
var hex = require("iotjs/encoding/hex")
var last = Date.now()
try {
    var path = "/dev/mtd10"
    var f = new mtd.File(path)
    console.log(f.info())
    var b = new Uint8Array(1024)
    f.readSync(b)
    f.close()
    console.log(hex.encodeToString(b))
    var db = new mtd.DB(path)
    try {
        for (var i = 0; i < 12; i++) {
            // db.setSync("ko" + i, new TextEncoder().encode("this is value " + i))
            var val = db.getSync("ko" + i)
            console.log('-------read', val === undefined ? undefined : new TextDecoder().decode(val))
        }
    } finally {
        db.close()
    }
} catch (e) {
    console.log(e instanceof Error ? e.toString() : e)
}
console.log("used", Date.now() - last + "ms")