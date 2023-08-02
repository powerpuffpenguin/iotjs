
declare namespace iotjs {
    // 編譯目標 os
    export const os: string
    // 編譯目標 arch
    export const arch: string
}
declare namespace deps {
    export interface URL {
        scheme: string
        userinfo?: string
        host: string
        port?: number
        path: string
        query?: string
        fragment?: string
    }
    /**
     * 解析 url
     * @param url 
     */
    export function http_parse_url(url: string): URL

    export function get_length(s: string | ArrayBuffer | Uint8Array): number
    export function socket_error(): string
    export interface TCPConnOptions {
        /**
         * 連接地址
         */
        addr: string
        /**
         * 連接端口
         */
        port: number
        /**
         * 如果爲 true 使用 tls
         */
        tls: boolean
        /**
         * tls 時使用的 
         * sni hostname
         */
        hostname?: string
        /**
         * 在使用 tls 連接時不驗證證書合法性
         */
        insecure?: boolean
        /**
         * 連接超時毫秒數
         */
        timeout: number
        /**
         *  當待讀取的 tcp 數據積壓到此值將停止接收數據
         */
        read?: number
        /**
         * 當待寫入的 tcp 數據積壓到此值，新的寫入將失敗
         */
        write?: number
    }
    export class TCPConn {
        __hash_tcp_conn: string
        private constructor()
        onEvent?: (what: number) => void
        onWrite?: () => void
        onRead?: () => void
        onError?: (e: any) => void
    }
    export function tcp_connect(opts: TCPConnOptions, cb: (conn: deps.TCPConn, e: any) => void): void
    /**
     * 手動釋放 conn
     */
    export function tcp_free(conn: TCPConn): void
    /**
     * 寫入數據 返回寫入字節數
     */
    export function tcp_write(conn: TCPConn, s: string | Uint8Array | ArrayBuffer): number
    /**
     * 讀取數據 返回寫入字節數
     */
    export function tcp_read(conn: TCPConn, data: Uint8Array | ArrayBuffer): number
    /**
     * 如果存在數據，儘量讀取可能多的數據
     */
    export function tcp_readMore(conn: TCPConn): Uint8Array | undefined
    /**
     * 返回是否可讀
     */
    export function tcp_readable(conn: TCPConn): number
    /**
     * 返回是否可寫
     */
    export function tcp_writable(conn: TCPConn): boolean

    /**
    * 爲底層設置 讀寫緩衝區大小
    * @param read 如果爲 true 設置 讀取緩衝區否則設置 寫入緩衝區
    */
    export function tcp_setBuffer(conn: TCPConn, read: boolean, n: number): void
    /**
     * 返回底層緩衝區大小
     */
    export function tcp_getBuffer(conn: TCPConn, read: boolean): number
    /**
     * 設置讀寫超時毫秒，如果 <= 0 則禁用超時回調
     */
    export function tcp_setTimeout(conn: TCPConn, read: number, write: number): void

    export function tcp_setPriority(conn: TCPConn, pri: number): void
    export function tcp_getPriority(conn: TCPConn): number

    /**
     * 爲 websocket 連接生成隨機密鑰
     */
    export function ws_key(): string
    export function http_expand_ws(conn: TCPConn, key: string, readlimit: number, cb: (resp: any, e: any) => void): void
    /**
     * 返回是否有就緒的 websocket 幀
     */
    export function ws_readable(conn: TCPConn): boolean
    export function ws_read(conn: TCPConn): string | Uint8Array | undefined
    export function ws_send(conn: TCPConn, s: string | Uint8Array | ArrayBuffer): boolean
}
const BEV_EVENT_READING = 0x01
// const BEV_EVENT_WRITING = 0x02
const BEV_EVENT_EOF = 0x10
const BEV_EVENT_ERROR = 0x20
const BEV_EVENT_TIMEOUT = 0x40
// const BEV_EVENT_CONNECTED = 0x80

const EV_READ = 0x2
const EV_WRITE = 0x4
export class NetError extends _iotjs.IotError {
    constructor(message?: string | undefined, options?: ErrorOptions | undefined) {
        super(message, options)
        this.name = "NetError"
    }
    eof?: boolean
    cancel?: boolean
}
function netError(e: any): never {
    if (typeof e === "string") {
        throw new NetError(e)
    }
    throw e
}
function getError(e: any): any {
    if (typeof e === "string") {
        return new NetError(e)
    }
    return e
}
function runSync<T>(f: () => T, exit?: boolean) {
    try {
        return f()
    } catch (e) {
        if (exit) {
            e = getError(e)
            if (e instanceof Error) {
                console.error(e.toString())
            } else {
                console.error(e)
            }
            _iotjs.exit(1)
        } else {
            netError(e)
        }
    }
}

const tcpLog = new _iotjs.Logger({
    prefix: "TCPConn"
})
export interface TCPConnOptions {
    /**
     * 如果爲 true 使用 tls
     */
    tls?: boolean
    /**
     * tls 時使用的 
     * sni hostname
     */
    hostname?: string
    /**
     * 在使用 tls 連接時不驗證證書合法性
     */
    insecure?: boolean
    /**
     * 連接超時毫秒數，小於 1 將不設置超時但通常系統 tcp 連接超時是 75s
     */
    timeout?: number
    /**
     *  當待讀取的 tcp 數據積壓到此值將停止接收數據
     */
    read?: number
    /**
     * 當待寫入的 tcp 數據積壓到此值，新的寫入將失敗
     */
    write?: number
}
export interface TCPConnHook {
    onWrite?: () => boolean
    onRead?: () => boolean
    onClear?: (e: any) => boolean
}
export class TCPConn {
    static connect(addr: string, port: number, arg0?: any, arg1?: any, arg2?: any): void {
        let opts: undefined | TCPConnOptions
        let cb: (conn?: TCPConn, e?: any) => void
        let hook: undefined | TCPConnHook
        if (typeof arg1 === "function") {
            opts = arg0
            cb = arg1
            hook = arg2
        } else {
            cb = arg0
            hook = arg1
        }
        const tls = opts?.tls ?? false;
        deps.tcp_connect({
            addr: addr,
            port: port,
            tls: tls,
            hostname: tls ? (opts?.hostname ?? addr) : undefined,
            insecure: tls ? (opts?.insecure ?? false) : undefined,
            timeout: opts?.timeout ?? 0,
            read: opts?.read ?? 1024 * 1024,
            write: opts?.write ?? 1024 * 1024,
        }, (conn, e) => {
            if (conn) {
                let ok: TCPConn
                try {
                    ok = new TCPConn(conn, hook)
                } catch (e) {
                    runSync(() => cb(undefined, e), true)
                    return
                }
                runSync(() => cb(ok), true)
            } else {
                runSync(() => cb(undefined, getError(e)), true)
            }
        })
    }
    protected constructor(readonly conn_: deps.TCPConn, readonly hook_?: TCPConnHook) {
        conn_.onEvent = (what) => {
            if (!this.closed_) {
                this._onEvent(what)
            }
        }
        conn_.onWrite = () => {
            if (!this.closed_) {
                this._onWrite()
            }
        }
        conn_.onRead = () => {
            if (!this.closed_) {
                this._onRead()
            }
        }
        conn_.onError = (e) => {
            if (!this.closed_) {
                this._onError(e)
            }
        }
    }
    debug = false
    private _onError(e: any) {
        if (this.debug) {
            tcpLog.log('_onError ${e}')
        }
        this.closed_ = -3
        const err = getError(e)
        if (this.onError) {
            this.onError(err)
        }
        deps.tcp_free(this.conn_)
        this._onClear(err)
        if (this.onClose) {
            this.onClose()
        }
    }
    /**
     * 設備關閉後自動回調，這個函數始終會被調用，你可以在此進行一些收尾的資源釋放工作
     * @remarks
     * 你不需要調用 this.close ，因爲連接資源已經被釋放之後才會調用此函數
     */
    onClose?: () => void
    /**
     * 連接出現錯誤時回調用於通知錯誤原因，如果是讀取到 eof 會傳入 undefined，否則傳入錯誤原因(通常是 NetError)
     * @remarks
     * 你不需要調用 this.close，在回調結束後系統會自動調用 this.close 釋放連接資源
     */
    onError?: (e?: any) => void
    /**
     * 當寫入緩衝區已滿，客戶端將變得不可寫，並且 write 會失敗，當客戶端再次變得可寫時會回調此函數
     */
    private w_?: () => void
    get onWritable(): undefined | (() => void) {
        return this.w_
    }
    set onWritable(f: undefined | (() => void)) {
        this.w_ = f
        if (!this.closed_ && deps.tcp_writable(this.conn_)) {
            this._onWrite()
        }
    }

    /**
     * 當連接變得可讀時回調
     */
    private r_?: () => void
    get onReadable(): undefined | (() => void) {
        return this.r_
    }
    set onReadable(f: undefined | (() => void)) {
        this.r_ = f
        if (!this.closed_ && deps.tcp_readable(this.conn_)) {
            this._onRead()
        }
    }
    /**
     * 爲底層設備設置 讀寫超時，只有在存在讀寫時才會調用此回調
     * @remarks
     * 例如當讀取緩衝區已滿，設備會自動停止接收網路數據這時不會調用讀取超時，因爲已經沒有讀取。
     * 類似如果寫入緩衝區爲空不存在寫入，這樣也不會調用寫入超時
     */
    onTimeout?: (read: boolean) => void
    /**
     * 當收到數據時回調
     */
    private m_?: (data: Uint8Array) => void
    get onMessage(): undefined | ((data: Uint8Array) => void) {
        return this.m_
    }
    set onMessage(f: undefined | ((data: Uint8Array) => void)) {
        this.m_ = f
        if (!this.closed_ && deps.tcp_readable(this.conn_)) {
            this._onRead()
        }
    }

    private closed_ = 0;
    private _onClear(e: any) {
        runSync(() => {
            const hook = this.hook_?.onClear
            if (hook && hook(e)) {
                return
            }
            let c = this.write_
            if (c && c.length) {
                this.write_ = undefined
                for (let i = 0; i < c.length; i++) {
                    c[i].cb(undefined, e)
                }
            }
            c = this.read_
            if (c && c.length) {
                this.read_ = undefined
                for (let i = 0; i < c.length; i++) {
                    c[i].cb(undefined, e)
                }
            }
        }, true)
    }
    private _onWrite() {
        runSync(() => {
            const onWrite = this.hook_?.onWrite
            if (onWrite) {
                if (onWrite()) {
                    return
                }
            }

            // 寫入未完成數據
            const c = this.write_
            if (c && c.length) {
                const length = c.length
                let i = 0;
                let node: Cancel
                let n: number
                for (; i < length; i++) {
                    node = c[i]
                    if (node.state_) {
                        continue
                    }

                    n = deps.tcp_write(this.conn_, node.data!)
                    if (n) {
                        // 通知寫入成功
                        node.cb(n)
                    } else {
                        // 依然沒有足夠緩存，回調前可能有其它寫入
                        if (i) {
                            c.splice(0, i)
                        }
                        return
                    }
                }
                if (i) {
                    c.splice(0, i)
                }
            }

            // 通知上層用戶
            const w = this.onWritable
            if (w && deps.tcp_writable(this.conn_)) {
                w()
            }
        })
    }
    private _onRead() {
        runSync(() => {
            const onRead = this.hook_?.onRead
            if (onRead) {
                if (onRead() || this.closed_) {
                    return
                }
            }
            // 讀取數據
            const c = this.read_
            if (c && c.length) {
                const length = c.length
                let i = 0;
                let node: Cancel
                let n: number
                for (; i < length; i++) {
                    node = c[i]
                    if (node.state_) {
                        continue
                    }

                    n = deps.tcp_read(this.conn_, node.data as Uint8Array)
                    if (n) {
                        // 通知讀取成功
                        node.cb(n)
                        if (this.closed_) {
                            return
                        }
                    } else {
                        // 依然沒有可寫，回調前可能有其它讀取
                        if (i) {
                            c.splice(0, i)
                        }
                        return
                    }
                }
                if (i) {
                    c.splice(0, i)
                }
            }

            // 通知上層用戶 有可讀數據
            const r = this.r_
            if (r) {
                if (!deps.tcp_readable(this.conn_)) {
                    return;
                }
                r()
                if (this.closed_) {
                    return
                }
            }

            // 通知上層用戶 有消息需要處理
            const m = this.m_
            if (m) {
                const data = deps.tcp_readMore(this.conn_)
                if (data) {
                    m(data)
                }
                return
            }
        }, true)
    }
    private _onEvent(what: number) {
        runSync(() => {
            if (this.debug) {
                tcpLog.log('_onEvent', what)
            }
            if (what & BEV_EVENT_EOF) {
                if (this.debug) {
                    tcpLog.log('eof')
                }

                this.closed_ = -1
                const err = new NetError("TCPConn already closed")
                err.eof = true
                if (this.onError) {
                    this.onError(err)
                }
                deps.tcp_free(this.conn_)
                this._onClear(err)
                if (this.onClose) {
                    this.onClose()
                }
                return
            }
            if (what & BEV_EVENT_TIMEOUT) {
                if (what & BEV_EVENT_READING) {
                    if (this.debug) {
                        tcpLog.log('reading timeout')
                    }
                    if (this.onTimeout) {
                        this.onTimeout(true)
                    }
                } else {
                    if (this.debug) {
                        tcpLog.log('writing timeout')
                    }
                    if (this.onTimeout) {
                        this.onTimeout(false)
                    }
                }
                return
            }
            const emsg = deps.socket_error()
            if (what & BEV_EVENT_ERROR) {
                if (what & BEV_EVENT_READING) {
                    if (this.debug) {
                        tcpLog.log('reading', emsg)
                    }
                } else {
                    if (this.debug) {
                        tcpLog.log('writing', emsg)
                    }
                }

                this.closed_ = -2
                const err = new NetError(emsg)
                if (this.onError) {
                    this.onError(err)
                }
                deps.tcp_free(this.conn_)
                this._onClear(err)
                if (this.onClose) {
                    this.onClose()
                }
                return
            }
        })
    }
    close(): void {
        if (this.closed_) {
            return
        }
        if (this.debug) {
            tcpLog.log('close')
        }
        this.closed_ = 1
        deps.tcp_free(this.conn_)
        this._onClear(new NetError("TCPConn already closed"))
        if (this.onClose) {
            this.onClose()
        }
    }
    /**
     * 返回設備是否已經關閉
     */
    get isClosed(): boolean {
        return this.closed_ ? true : false
    }
    /**
     * 嘗試寫入數據，返回實際寫入字節數
     * @param s 
     */
    tryWrite(s: string | Uint8Array | ArrayBuffer): number | undefined {
        if (this.closed_) {
            throw new NetError("TCPConn already closed")
        }
        if (!deps.get_length(s)) {
            return 0
        }
        try {
            const n = deps.tcp_write(this.conn_, s)
            return n == 0 ? undefined : n
        } catch (e) {
            netError(e)
        }
    }

    write(s: string | Uint8Array | ArrayBuffer, cb?: (n?: number, e?: any) => void): number | Cancel {
        const n = this.tryWrite(s)
        if (n !== undefined) {
            return n
        }
        const node = new Cancel(s, cb)
        let w = this.write_
        if (w) {
            w.push(node)
        } else {
            this.write_ = [node]
        }
        return node
    }
    private write_?: Array<Cancel>
    /**
     * 嘗試讀取數據，返回實際讀取的字節數
     */
    tryRead(s: Uint8Array | ArrayBuffer): number | undefined {
        if (this.closed_) {
            throw new NetError("TCPConn already closed")
        }
        if (!deps.get_length(s)) {
            return 0
        }
        try {
            const n = deps.tcp_read(this.conn_, s)
            return n == 0 ? undefined : n
        } catch (e) {
            netError(e)
        }
    }

    read(s: Uint8Array | ArrayBuffer, cb?: (n?: number, e?: any) => void): number | Cancel {
        const n = this.tryRead(s)
        if (n !== undefined) {
            return n
        }
        const node = new Cancel(s, cb)
        let r = this.read_
        if (r) {
            r.push(node)
        } else {
            this.read_ = [node]
        }
        return node
    }
    private read_?: Array<Cancel>

    get readable(): boolean {
        if (this.closed_) {
            return false
        }
        return deps.tcp_readable(this.conn_) ? true : false
    }
    get writable(): boolean {
        if (this.closed_) {
            return false
        }
        return deps.tcp_writable(this.conn_)
    }

    /**
    * 爲底層設置 讀寫緩衝區大小
    * @param read 如果爲 true 設置 讀取緩衝區否則設置 寫入緩衝區
    */
    setBuffer(read: boolean, n: number): void {
        if (this.closed_) {
            throw new NetError("TCPConn already closed")
        }
        deps.tcp_setBuffer(this.conn_, read, n)
    }
    /**
     * 返回底層緩衝區大小
     */
    getBuffer(read: boolean): number {
        if (this.closed_) {
            throw new NetError("TCPConn already closed")
        }
        return deps.tcp_getBuffer(this.conn_, read)
    }
    /**
     * 設置讀寫超時毫秒，如果 <= 0 則禁用超時回調
     */
    setTimeout(read: number, write: number): void {
        if (this.closed_) {
            throw new NetError("TCPConn already closed")
        }
        deps.tcp_setTimeout(this.conn_, read, write)
        this.tr_ = read
        this.tw_ = write
    }
    private tr_ = 0
    private tw_ = 0
    /**
     * 返回讀取/寫超時毫秒數，爲 0 表示禁用超時回調
     */
    getTimeout(): [number, number] {
        return [this.tr_, this.tw_]
    }
    setPriority(pri: number) {
        if (this.closed_) {
            throw new NetError("TCPConn already closed")
        }
        deps.tcp_setPriority(this.conn_, pri)
    }
    getPriority(): number {
        if (this.closed_) {
            throw new NetError("TCPConn already closed")
        }
        return deps.tcp_getPriority(this.conn_)
    }
}
export class Cancel {
    constructor(
        readonly data?: string | Uint8Array | ArrayBuffer,
        readonly cb_?: (n?: any, e?: any) => void,
    ) { }
    state_?: number
    cancel(e?: any): void {
        if (this.state_) {
            return
        }
        this.state_ = -1

        const cb = this.cb_
        if (cb) {
            if (e) {
                e = getError(e)
            } else {
                e = new NetError("write cancel")
                e.cancel = true
            }
            runSync(() => cb(undefined, e), true)
        }
    }
    cb(n?: any, e?: any) {
        if (this.state_) {
            return
        }
        this.state_ = 1

        const cb = this.cb_
        if (cb) {
            runSync(() => cb(n, e), true)
        }
    }
}
export interface WebsocketConnOptions {
    /**
     * 如果設置了此值，將直接連接此地址，而非從 url 解析 服務器地址
     */
    addr?: string
    /**
     * 如果設置了此值，將直接連接此端口，而非從 url 解析 服務器端口
     */
    port?: number
    /**
     * 可設置此屬性覆蓋連接的 header Origin
     */
    origin?: string
    /**
     * 可設置此屬性覆蓋連接的 header Host
     */
    host?: string
    /**
     * 在使用 tls 連接時不驗證證書合法性
     */
    insecure?: boolean
    /**
     * 連接超時毫秒數，小於 1 將不設置超時但通常系統 tcp 連接超時是 75s
     */
    timeout?: number
    /**
     * 當待讀取的 tcp 數據積壓到此值將停止接收數據
     * 默認爲 1024*1024
     */
    read?: number
    /**
     * 當待寫入的 tcp 數據積壓到此值，新的寫入將失敗
     * 默認爲 1024*1024
     */
    write?: number
    /**
     * 讀取到的單個消息最大長度
     * 默認爲 1024*1024
     */
    readlimit?: number
}
class WebsocketConnect {
    constructor(readonly opts: {
        wss: boolean,
        addr: string,
        sni: string,
        port: number,
        key: string,
        handshake: string,
        hook: TCPConnHook,
    }, readonly ws?: WebsocketConnOptions) { }
    private t_: any
    private cb: undefined | ((conn?: TCPConn, e?: any) => void)
    begin(cb: (conn?: TCPConn, e?: any) => void) {
        this.cb = cb
        const opts = this.opts
        const wss = opts.wss
        const ws = this.ws
        const timeout = ws?.timeout ?? 0
        if (timeout > 0) {
            this.t_ = setTimeout(() => {
                this._reject(wss ? new NetError("wss connect timeout") : new NetError("ws connect timeout"), true)
            }, timeout)
        }
        TCPConn.connect(ws?.addr ?? opts.addr, ws?.port ?? opts.port, {
            tls: opts.wss,
            insecure: opts?.wss ? ws?.insecure ?? false : undefined,
            hostname: opts?.wss ? opts.sni : undefined,
            read: ws?.read,
            write: ws?.write,
        }, (conn?: TCPConn, e?: any) => {
            if (conn) {
                try {
                    this.conn_ = conn
                    const cb = () => {
                        try {
                            deps.http_expand_ws(conn.conn_, opts.key, ws?.readlimit ?? 1024 * 1024, (e) => {
                                if (e) {
                                    this._reject(e)
                                } else {
                                    this._resolve(conn)
                                }
                            })
                        } catch (e) {
                            this._reject(e)
                        }
                    }
                    if (typeof conn.write(opts.handshake, cb) === "number") {
                        cb()
                    }
                } catch (e) {
                    this._reject(e)
                }
            } else {
                this._reject(e)
            }
        }, opts.hook)
    }
    private _reject(e: any, timeout?: boolean) {
        let x: any = this.t_
        if (x && !timeout) {
            this.t_ = undefined
            clearTimeout(x)
        }
        x = this.conn_
        if (x) {
            this.conn_ = undefined
            x.close()
        }
        this.cb!(undefined, getError(e))
    }
    private conn_?: TCPConn
    private _resolve(conn: TCPConn) {
        const t = this.t_
        if (t) {
            clearTimeout(t)
        }
        this.cb!(conn)
    }
}
export class WebsocketConn {
    /**
    * 連接 socket 服務器
    * @param hostname 連接ip或域名
    * @param port 連接端口
    * @param opts 
    */
    static connect(url: string, arg1?: any, arg2?: any): void {
        let opts: undefined | WebsocketConnOptions
        let cb: (conn?: WebsocketConn, e?: any) => void
        if (typeof arg1 === "function") {
            cb = arg1
        } else {
            opts = arg1
            cb = arg2
        }

        const u = deps.http_parse_url(url)
        let port = u.port
        let host: string
        let origin: string
        let wss: boolean
        if (u.scheme == "ws") {
            wss = false
            if (port) {
                host = opts?.host ?? `${u.host}:${port}`
            } else {
                host = opts?.host ?? u.host
                port = 80
            }
            origin = opts?.origin ?? `http://${host}`
        } else if (u.scheme == "wss") {
            wss = true
            if (port) {
                host = opts?.host ?? `${u.host}:${port}`
            } else {
                host = opts?.host ?? u.host
                port = 443
            }
            origin = opts?.origin ?? `https://${host}`
        } else {
            throw new Error(`scheme not supported: ${u.scheme}`);
        }

        const key = deps.ws_key()
        const path = u.query === undefined ? u.path : `${u.path}?${u.query}`
        const handshake = `GET ${path} HTTP/1.1
Host: ${host}
User-Agent: iotjs-${iotjs.os}-${iotjs.arch}-client/1.1
Origin: ${origin}
Connection: Upgrade
Upgrade: websocket
Sec-WebSocket-Version: 13
Sec-WebSocket-Key: ${key}

`
        const hook = {}
        const connect = new WebsocketConnect({
            wss: wss,
            addr: u.host,
            port: port,
            sni: opts?.host ?? u.host,
            key: key,
            handshake: handshake,
            hook: hook,
        }, opts)

        connect.begin((conn, e) => {
            if (conn) {
                let cc: WebsocketConn
                try {
                    cc = new WebsocketConn(conn, hook)
                } catch (e) {
                    runSync(() => cb(undefined, getError(e)), true)
                    return
                }
                runSync(() => cb(cc), true)
            } else {
                runSync(() => cb(undefined, e), true)
            }
        })
    }
    private constructor(private readonly conn_: TCPConn, hook: TCPConnHook) {
        hook.onWrite = () => {
            // 寫入未完成數據
            const c = this.send_
            if (c && c.length) {
                const length = c.length
                let i = 0;
                let node: Cancel
                let ok: boolean
                for (; i < length; i++) {
                    node = c[i]
                    if (node.state_) {
                        continue
                    }

                    ok = deps.ws_send(this.conn_.conn_, node.data!)
                    if (ok) {
                        // 通知寫入成功
                        node.cb(ok)
                    } else {
                        // 依然沒有足夠緩存，回調前可能有其它寫入
                        if (i) {
                            c.splice(0, i)
                        }
                        return true
                    }
                }
                if (i) {
                    c.splice(0, i)
                }
            }

            const w = this.conn_.onWritable
            if (w && deps.tcp_writable(this.conn_.conn_)) {
                w()
            }
            return true
        }
        hook.onRead = () => {
            // 讀取數據
            const c = this.recv_
            if (c && c.length) {
                const length = c.length
                let i = 0;
                let node: Cancel
                let data: string | Uint8Array | undefined
                for (; i < length; i++) {
                    node = c[i]
                    if (node.state_) {
                        continue
                    }

                    data = deps.ws_read(this.conn_.conn_)
                    if (data) {
                        // 通知讀取成功
                        node.cb(data)
                        if (this.conn_.isClosed) {
                            return true
                        }
                    } else {
                        // 依然沒有可寫，回調前可能有其它讀取
                        if (i) {
                            c.splice(0, i)
                        }
                        return true
                    }
                }
                if (i) {
                    c.splice(0, i)
                }
            }

            // 通知上層用戶 有可讀數據
            const r = this.onReadable
            if (r) {
                if (!deps.ws_readable(this.conn_.conn_)) {
                    return true
                }
                r()
                if (this.conn_.isClosed) {
                    return true
                }
            }

            // 通知上層用戶 有消息需要處理
            const m = this.onMessage
            if (m) {
                const data = deps.ws_read(this.conn_.conn_)
                if (data) {
                    m(data)
                }
                return true
            }
            return true
        }
        hook.onClear = (e) => {
            let c = this.send_
            if (c && c.length) {
                this.send_ = undefined
                for (let i = 0; i < c.length; i++) {
                    c[i].cb(undefined, e)
                }
            }
            c = this.recv_
            if (c && c.length) {
                this.recv_ = undefined
                for (let i = 0; i < c.length; i++) {
                    c[i].cb(undefined, e)
                }
            }
            return true
        }
    }
    set onClose(f: any) {
        this.conn_.onClose = f
    }
    get onClose() {
        return this.conn_.onClose
    }
    set onError(f: any) {
        this.conn_.onError = f
    }
    get onError() {
        return this.conn_.onError
    }
    set onWritable(f: any) {
        this.conn_.onWritable = f
    }
    get onWritable() {
        return this.conn_.onWritable
    }
    get writable() {
        return this.conn_.writable
    }
    set onReadable(f: any) {
        this.conn_.onReadable = f
    }
    get onReadable() {
        return this.conn_.onReadable
    }
    get readable() {
        if (this.conn_.isClosed) {
            return false
        }
        return deps.ws_readable(this.conn_.conn_) ? true : false
    }
    set onTimeout(f: any) {
        this.conn_.onTimeout = f
    }
    get onTimeout() {
        return this.conn_.onTimeout
    }
    close(): void {
        this.conn_.close()
    }
    isClosed(): boolean {
        return this.conn_.isClosed
    }
    setBuffer(read: boolean, n: number): void {
        this.conn_.setBuffer(read, n)
    }
    getBuffer(read: boolean): number {
        return this.conn_.getBuffer(read)
    }
    setTimeout(read: number, write: number): void {
        this.conn_.setTimeout(read, write)
    }
    getTimeout(): [number, number] {
        return this.conn_.getTimeout()
    }
    set onMessage(f: any) {
        this.conn_.onTimeout = f
    }
    get onMessage() {
        return this.conn_.onTimeout
    }
    trySend(data: string | Uint8Array | ArrayBuffer): boolean {
        if (this.conn_.isClosed) {
            throw new NetError("TCPConn already closed")
        }
        return runSync(() => deps.ws_send(this.conn_.conn_, data), true)
    }
    send(data: string | Uint8Array | ArrayBuffer, cb?: (ok?: boolean, e?: any) => void): boolean | Cancel {
        if (this.trySend(data)) {
            return true
        }
        const node = new Cancel(data, cb)
        let w = this.send_
        if (w) {
            w.push(node)
        } else {
            this.send_ = [node]
        }
        return node
    }
    private send_?: Array<Cancel>
    tryRecv(): undefined | string | Uint8Array {
        if (this.conn_.isClosed) {
            throw new NetError("TCPConn already closed")
        }
        return runSync(() => deps.ws_read(this.conn_.conn_), true)
    }
    recv(cb?: (data?: string | Uint8Array, e?: any) => void): string | Uint8Array | Cancel {
        const data = this.tryRecv()
        if (data !== undefined) {
            return data
        }

        const node = new Cancel(undefined, cb)
        let r = this.recv_
        if (r) {
            r.push(node)
        } else {
            this.recv_ = [node]
        }
        return node
    }

    private recv_?: Array<Cancel>

    setPriority(pri: number) {
        if (this.conn_.isClosed) {
            throw new NetError("TCPConn already closed")
        }
        deps.tcp_setPriority(this.conn_.conn_, pri)
    }
    getPriority(): number {
        if (this.conn_.isClosed) {
            throw new NetError("TCPConn already closed")
        }
        return deps.tcp_getPriority(this.conn_.conn_)
    }
}
