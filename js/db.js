var mtd = require("iotjs/mtd")
var last = Date.now()
try {
    var path = "/dev/mtd5"
    var db = new mtd.DB(path)
    try {
        console.log(db.info())
        for (var i = 0; i < 12; i++) {
            var key = "ko" + i
            db.setSync(key, new TextEncoder().encode("this is value " + i))
            if (i % 3 == 1) {
                db.deleteSync(key)
            }
            console.log(key, db.hasSync(key))
            var val = db.getSync(key)
            console.log('read ' + i + ':', val ? new TextDecoder().decode(val) : val)
        }
        db.keysSync(function (key) {
            // console.log("*", key)
        })
        // db.foreachSync(function (key, data) {
        //     console.log("-", key + "=" + new TextDecoder().decode(data))
        // })
    } finally {
        db.close()
    }
} catch (e) {
    console.log(e instanceof Error ? e.toString() : e)
}
console.log("used", Date.now() - last + "ms")