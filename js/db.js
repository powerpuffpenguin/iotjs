var mtd = require("iotjs/mtd")
var hex = require("iotjs/encoding/hex")
var last = Date.now()
try {
    var path = "/dev/mtd5"
    var db = new mtd.DB(path)
    try {
        console.log(db.info())
        for (var i = 0; i < 12; i++) {
            db.setSync("ko" + i, "this is value " + i)
            console.log("ko" + i, db.hasSync("ko" + i))
            var val = db.getStringSync("ko" + i)
            console.log('-------read', val)
        }
    } finally {
        db.close()
    }
} catch (e) {
    console.log(e instanceof Error ? e.toString() : e)
}
console.log("used", Date.now() - last + "ms")