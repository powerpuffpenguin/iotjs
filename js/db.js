var mtd = require("iotjs/mtd")
var last = Date.now()
try {
    var path = "/dev/mtd5"
    var db = new mtd.DB(path)
    try {
        console.log(db.info())
        for (var i = 0; i < 12; i++) {
            var key = "ko" + i
            db.setSync(key, "this is value " + i)
            if (i % 3 == 1) {
                db.deleteSync(key)
            }
            console.log(key, db.hasSync(key))
            var val = db.getStringSync(key)
            console.log('read ' + i + ':', val)
        }
    } finally {
        db.close()
    }
} catch (e) {
    console.log(e instanceof Error ? e.toString() : e)
}
console.log("used", Date.now() - last + "ms")