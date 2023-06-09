
var mtd = require("iotjs/mtd")
var coroutine = require("iotjs/coroutine")

function read(co, src, data) {
    return co.yield(function (notify) {
        src.read(data, function (ret, e) {
            if (ret === undefined) {
                notify.error(e)
            } else {
                notify.value(ret)
            }
        })
    })
}
coroutine.go(function (co) {
    var last = Date.now()
    try {
        var f = new mtd.File("/dev/mtd2")
        try {
            console.log(f.info())

            var b = new Uint8Array(1024)
            while (true) {
                console.log("call read")
                var n = read(co, f, b)
                if (!n) {
                    break
                }
                console.log("read", n)
            }
        } finally {
            f.close()
        }
    } catch (e) {
        console.log(e instanceof Error ? e.toString() : e)
    }
    console.log(Date.now() - last)
})
