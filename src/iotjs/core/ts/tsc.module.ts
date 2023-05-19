const path = _iotjs.path

function resolve_js_module(id: string): string | undefined {
    // 明確指定了加載 js
    if (id.endsWith(".js")) {
        if (_iotjs.stat_module(id) == 2) {
            return id
        }
    }

    // 優先加載 js 檔案
    let s = id + ".js"
    if (_iotjs.stat_module(s)) {
        return s
    }

    // 作爲目錄加載
    if (_iotjs.stat_module(id) != 1) {
        return
    }

    // 優先加載 index.js
    s = id + "/index.js"
    if (_iotjs.stat_module(s) == 2) {
        return s
    }
    // 解析 package.json
    s = id + "/package.json"
    if (_iotjs.stat_module(s) != 2) {
        return s
    }
    let obj: any
    try {
        obj = JSON.parse(_iotjs.read_text(s))
    } catch (e) {
        throw new Error(`parse ${s} error: ${e}`);
    }
    if (typeof obj.index === "string" && obj.index != '') {
        s = path.join(id, obj.index)
    } else if (typeof obj.main === "string" && obj.index != '') {
        s = path.join(id, obj.main)
    } else {
        throw new Error(`unknow type file: ${s}`)
    }
    if (_iotjs.stat_module(s) == 2) {
        return s
    }
    if (s.endsWith(".js")) {
        throw new Error(`module file not exits: ${s}`);
    }
    s += ".js"
    if (_iotjs.stat_module(s) == 2) {
        return s
    }
    throw new Error(`module file not exits: ${s}`);
}
export function resolve_module(pid: string, native: Record<string, boolean>, id: string, debug?: boolean): string {
    if (debug) {
        console.log(`resolve_module pid=${pid} id=${id}`)
    }
    if (typeof id != "string" || id == '') {
        throw new Error(`require id invalid: ${id}`)
    } else if (typeof pid != "string" || pid == '') {
        throw new Error(`require pid invalid: ${pid}`)
    }

    if (id.startsWith('.')) {
        const s = resolve_js_module(path.join(path.dir(pid), id))
        if (!s) {
            throw new Error(`require id not found: ${id}`)
        }
        return s
    }
    id = path.clean(id);
    // 優先加載系統模塊
    if (native[id]) {
        return id
    }
    if (!path.isAbs(pid)) {
        // 系統模塊不允許加載非系統模塊
        throw new Error(`native module cannot load node_modules: pid=${pid} id=${id}`)
    }
    // 從當前目錄向上查找 node_modules
    let dir = path.dir(pid)
    let s = resolve_js_module(path.join(dir, 'node_modules', id))
    if (s) {
        return s
    }
    while (dir != "/" && dir != "" && dir != ".") {
        s = resolve_js_module(path.join(dir, 'node_modules', id))
        if (s) {
            return s
        }
        dir = path.dir(dir)
    }
    throw new Error(`require id not found: ${id}`)
}
