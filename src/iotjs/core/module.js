(function () {
    String.prototype.padEnd = function (m, f) {
        if (f === null || f === undefined || f === '') {
            f = " "
        } else {
            f = f.toString()
            if (f == "") {
                f = " "
            }
        }
        if (typeof m === "string") {
            m = parseInt(m)
        } else if (typeof m !== "number") {
            return this
        }
        m = Math.floor(m)
        if (!Number.isFinite(m) || this.length >= m) {
            return this
        }
        var s = this
        while (s.length < m) {
            s += f
        }
        if (s.length > m) {
            s = s.slice(0, m)
        }
        return s
    }
    String.prototype.padStart = function (m, f) {
        if (f === null || f === undefined || f === '') {
            f = " "
        } else {
            f = f.toString()
            if (f == "") {
                f = " "
            }
        }
        if (typeof m === "string") {
            m = parseInt(m)
        } else if (typeof m !== "number") {
            return this
        }
        m = Math.floor(m)
        if (!Number.isFinite(m) || this.length >= m) {
            return this
        }
        var s = this
        while (s.length < m) {
            s += f
        }
        if (s.length > m) {
            s = s.slice(0, m)
        }
        return s
    }
    var exports = {}
    exports.resolve = function (pid, native, c, id) {
        if (id == '') {
            throw new Error("require id invalid")
        }
        var path = c.path
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
})();