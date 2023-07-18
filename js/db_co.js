var mtd = require("iotjs/mtd")
var coroutine = require("iotjs/coroutine")
coroutine.go(function (co) {
    var last = Date.now()
    try {
        var path = "/dev/mtd5"
        var db = new mtd.DB(path)
        try {
            console.log(db.info())
            for (var i = 0; i < 12; i++) {
                co.yield(function (notify) {
                    db.set("ko" + i, "this is value " + i, function (e) {
                        if (e) {
                            notify.error(e)
                        } else {
                            notify.value()
                        }
                    })
                })
                var exists = co.yield(function (notify) {
                    db.has("ko" + i, function (ret, e) {
                        if (ret === undefined) {
                            notify.error(e)
                        } else {
                            notify.value(ret)
                        }
                    })
                })
                console.log("ko" + i, exists)
                var val = co.yield(function (notify) {
                    db.getString("ko" + i, function (ret, e) {
                        if (e) {
                            notify.error(e)
                        } else {
                            notify.value(ret)
                        }
                    })
                })
                console.log('-------read', val)
            }
        }
        finally {
            db.close()
        }
    } catch (e) {
        console.log(e instanceof Error ? e.toString() : e)
    }
    console.log("used", Date.now() - last + "ms")
})