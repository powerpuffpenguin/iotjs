var mtd = require("iotjs/mtd")
var last = Date.now()
try {
    var f = new mtd.File("/dev/mtd2")
    try {
        console.log(f.info())
        // var b = new Uint8Array(1024)
        // while (true) {
        //     var n = f.readSync(b)
        //     if (!n) {
        //         break
        //     }
        //     console.log("read", n)
        // }
    } finally {
        f.close()
    }
} catch (e) {
    console.log(e instanceof Error ? e.toString() : e)
}
console.log(Date.now() - last)