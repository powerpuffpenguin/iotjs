(function () {
    var exports = {}
    exports.resolve_module = function (id, pid, path, native, c) {
        if (id == '') {
            throw new Error("require id invalid")
        }
        if (id.startsWith('.')) {
            if (typeof pid === "string") {
                id = path.join(path.dir(pid), id)
            } else {
                id = path.clean(id)
            }
        } else {
            id = path.clean(id)
            if (native[id]) {
                return id
            }
            id = path.join("node_modules", id)
        }
        var stat = c.stat(id)
        if (stat && (stat.mode & 0x1)) {
            id = path.join(id, "index.js")
        } else {
            id += ".js"
        }
        return id
    }
    return exports
})()