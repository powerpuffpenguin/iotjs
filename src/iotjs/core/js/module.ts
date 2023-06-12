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
export class IotError extends Error {
    constructor(message: string, options?: ErrorOptions | undefined) {
        super(message, options)
        // restore prototype chain   
        const proto = new.target.prototype
        if (Object.setPrototypeOf) {
            Object.setPrototypeOf(this, proto)
        }
        else {
            (this as any).__proto__ = proto
        }
        this.name = "IotError"
    }
}
/**
 * Create a Promise with a completion marker
 * 
 * @remarks
 * Completer is inspired by dart, it can be used to return the same piece of data for the same asynchronous request. For example, you have a database singleton that only returns the same connection when the database is actually requested.
 * 
 * @example
 * Suppose we have a connect function that creates a connection to the database and its signature looks like this `function connectDB():Promise<DB>`.
 * We can use the Completer wrapper to create only one connection
 * ```
* class Helper {
 *     private constructor() { }
 *     private static db_?: Completer<DB>
 *     static async db(): Promise<DB> {
 *         const db = Helper.db_
 *         if (db) {
 *             // connecting, return the promise of the connection
 *             return db.promise
 *         }
 *         // no connection, create a new connection
 *         const completer = new Completer<DB>()
 *         Helper.db_ = completer // locked
 *         try {
 *             const val = await connectDB()
 *             completer.resolve(val) // complete success
 *         } catch (e) {
 *             Helper.db_ = undefined // unlocked to allow subsequent calls to retry the operation
 * 
 *             completer.reject(e) // complete error
 *         }
 *         return completer.promise
 *     }
 * }
 * ```
 */
export class Completer<T>{
    private promise_: Promise<T> | undefined
    private resolve_: ((value?: T | PromiseLike<T>) => void) | undefined
    private reject_: ((reason?: any) => void) | undefined
    private c_ = false
    /**
     * Returns whether the Promise has completed
     */
    get isCompleted(): boolean {
        return this.c_
    }
    constructor() {
        this.promise_ = new Promise<T>((resolve, reject) => {
            this.resolve_ = resolve as any
            this.reject_ = reject
        })
    }
    /**
     * Returns the created Promise
     */
    get promise(): Promise<T> {
        return this.promise_ as Promise<T>
    }
    /**
     * Complete success, Promise.resolve(value) 
     * @param value 
     */
    resolve(value?: T | PromiseLike<T>): void {
        if (this.c_) {
            return
        }
        this.c_ = true
        if (this.resolve_) {
            this.resolve_(value)
        }
    }
    /**
     * Complete error, Promise.reject(reason) 
     * @param reason 
     */
    reject(reason?: any): void {
        if (this.c_) {
            return
        }
        this.c_ = true
        if (this.reject_) {
            this.reject_(reason)
        }
    }
}


export interface OutputOptions {
    /**
     * prefix label
     */
    prefix: string
    /**
     * println current datetime
     */
    time: boolean
}
export interface Output {
    log(opts: OutputOptions, vals: Array<any>): void
}
export function pad(v: number, len: number): string {
    return v.toString().padStart(len, '0')
}
export const defaultOutput = {
    log(opts: OutputOptions, vals: Array<any>): void {
        let prefix = ''
        if (opts.prefix != '') {
            prefix = `[${opts.prefix}]`
        }
        if (opts.time) {
            const d = new Date()
            const str = `[${d.getFullYear()}/${pad(d.getMonth(), 2)}/${pad(d.getDay(), 2)} ${pad(d.getHours(), 2)}:${pad(d.getMinutes(), 2)}:${pad(d.getSeconds(), 2)}]`
            if (prefix == '') {
                prefix = str
            } else {
                prefix = prefix + " " + str
            }
        }

        if (prefix === '') {
            console.log(...vals)
        } else {
            console.log(prefix, ...vals)
        }
    },
}
export interface LoggerOptionsInit {
    /**
     * Log output target
     */
    output?: Output,

    /**
     * Whether to enable logging
     */
    enable?: boolean
    /**
     * prefix label
     */
    prefix?: string
    /**
     * println current datetime
     */
    time?: boolean
}
export interface LoggerOptions extends OutputOptions {
    /**
     * Log output target
     */
    output: Output
    /**
     * Whether to enable logging
     */
    enable: boolean
}

export class Logger {
    public readonly opts: LoggerOptions

    constructor(opts?: LoggerOptionsInit) {
        const {
            output = defaultOutput,
            enable = true,
            prefix = '',
            time = true,
        } = opts ?? {}

        this.opts = {
            output: output,
            enable: enable,
            prefix: prefix,
            time: time,
        }
    }
    log(...vals: Array<any>): void {
        const opts = this.opts
        if (opts.enable) {
            opts.output.log(opts, vals)
        }
    }
}
export const defaultLogger = new Logger({
    prefix: 'iotjs'
})