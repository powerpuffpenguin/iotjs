var mtd = require("iotjs/mtd")
try {
    var f = new mtd.File("/dev/mtd2")
    try {
        console.log(f.info())
        f.eraseSync(f.info().erasesize, f.info().erasesize * 100)

    } finally {
        f.close()
    }
} catch (e) {
    console.log(e instanceof Error ? e.toString() : e)
}
