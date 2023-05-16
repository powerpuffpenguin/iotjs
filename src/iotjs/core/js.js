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
