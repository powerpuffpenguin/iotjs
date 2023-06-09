
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
function runSync<T>(f: () => T) {
    try {
        return f()
    } catch (e) {
        netError(e)
    }
}
async function runAsync<T>(f: () => Promise<T>) {
    try {
        return await f()
    } catch (e) {
        netError(e)
    }
}
export class IP {
    static fromIP4(s: string): IP {
        return runSync(() => new IP(_iotjs.dns.parseIP4(s)));
    }
    static fromIP6(s: string): IP {
        return runSync(() => new IP(_iotjs.dns.parseIP6(s)));
    }

    constructor(public readonly ip: Uint8Array) {
        if (ip.length != 4 && ip.length != 16) {
            throw new NetError(`buffer not a ip4 or ip6`)
        }
    }
    get ip4(): boolean {
        return this.ip.length == 4
    }
    get ip6(): boolean {
        return this.ip.length == 16
    }
    toString(): string {
        return runSync(() => {
            return _iotjs.dns.ipToString(this.ip)
        })
    }
    string(): string {
        return runSync(() => {
            return `${this.ip4 ? 'ip4' : 'ip6'}:[${_iotjs.dns.ipToString(this.ip)}]`
        })
    }
}
export function resolveIP(network: 'ip' | 'ip4' | 'ip6', address: string): Promise<Array<IP>> {
    return runAsync(async () => {
        let ip: Array<Uint8Array> | undefined
        switch (network) {
            case 'ip':
                ip = await new Promise<Array<Uint8Array>>((resolve, reject) => {
                    let err = false
                    _iotjs.dns.resolveIP4(address).then((v) => {
                        resolve(v)
                    }).catch((e) => {
                        if (err) {
                            reject(e)
                        } else {
                            err = true
                        }
                    })
                    _iotjs.dns.resolveIP6(address).then((v) => {
                        resolve(v)
                    }).catch((e) => {
                        if (err) {
                            reject(e)
                        } else {
                            err = true
                        }
                    })
                })
                break
            case 'ip4':
                ip = await _iotjs.dns.resolveIP4(address)
                break
            case 'ip6':
                ip = await _iotjs.dns.resolveIP6(address)
                break
            default:
                throw new NetError(`network invalid: ${network}`)
        }
        return ip.map((v) => new IP(v))
    })
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
    onRead?: () => boolean
}
export class TCPConn {
    static connect(addr: string, port: number, opts?: TCPConnOptions, hook?: TCPConnHook): Promise<TCPConn> {
        return runSync(() => {
            const tls = opts?.tls ?? false;
            return new Promise<TCPConn>((resolve, reject) => {
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
                            reject(e)
                            return
                        }
                        resolve(ok)
                    } else {
                        reject(getError(e))
                    }
                })
            })
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
        let c = this.write_
        if (c) {
            this.write_ = undefined
            this.writeData_ = undefined
            c.reject(e)
        }
        c = this.read_
        if (c) {
            this.read_ = undefined
            this.readData_ = undefined
            c.reject(e)
        }
    }
    private _onWrite() {
        let writable = true
        // 寫入未完成數據
        const c = this.write_
        if (c) {
            writable = false
            this.write_ = undefined
            const data = this.writeData_!
            this.writeData_ = undefined
            try {
                const n = deps.tcp_write(this.conn_, data)
                if (n) {
                    c.resolve(n)
                } else {
                    // 依然沒有足夠緩存，回調前可能有其它寫入
                    this.write_ = c
                    this.writeData_ = data
                }
            } catch (e) {
                c.reject(getError(e))
            }
        }

        // 通知上層用戶
        if (this.onWritable) {
            if (!writable && !deps.tcp_writable(this.conn_)) {
                return
            }
            this.onWritable()
        }
    }
    private _onRead() {
        const onRead = this.hook_?.onRead
        if (onRead) {
            if (onRead()) {
                return
            }
        }
        let readable = true
        // 讀取數據
        const c = this.read_
        if (c) {
            readable = false
            this.read_ = undefined
            const data = this.readData_!
            this.readData_ = undefined
            try {
                const n = deps.tcp_read(this.conn_, data)
                if (n) {
                    c.resolve(n)
                } else {
                    // 依然沒有數據，回調前可能有其它讀取
                    this.read_ = undefined
                    this.readData_ = data
                }
            } catch (e) {
                c.reject(getError(e))
            }
        }

        // 通知上層用戶 有可讀數據
        const r = this.r_
        if (r) {
            if (!readable && !deps.tcp_readable(this.conn_)) {
                return;
            }
            readable = false;
            r()
        }

        // 通知上層用戶 有消息需要處理
        const m = this.m_
        if (m) {
            if (!readable && !deps.tcp_readable(this.conn_)) {
                return;
            }
            const data = deps.tcp_readMore(this.conn_)
            if (data) {
                m(data)
            }
            return
        }
    }
    private _onEvent(what: number) {
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
    tryWrite(s: string | Uint8Array | ArrayBuffer): number {
        if (this.closed_) {
            throw new NetError("TCPConn already closed")
        }
        if (!deps.get_length(s)) {
            return 0
        }
        try {
            return deps.tcp_write(this.conn_, s)
        } catch (e) {
            netError(e)
        }
    }

    write(s: string | Uint8Array | ArrayBuffer): number | Promise<number> {
        if (this.closed_) {
            throw new NetError("TCPConn already closed")
        }
        if (!deps.get_length(s)) {
            return 0
        }
        const n = deps.tcp_write(this.conn_, s)
        if (n) {
            return n
        }
        return this._write(s)
    }
    private write_: _iotjs.Completer<number> | undefined
    private writeData_: string | Uint8Array | ArrayBuffer | undefined
    private async _write(s: string | Uint8Array | ArrayBuffer): Promise<number> {
        // 等待未完成寫入
        let c = this.write_
        while (c) {
            await c
            if (this.closed_) {
                throw new NetError("TCPConn already closed")
            }
            c = this.write_
        }

        // 鎖定寫入
        c = new _iotjs.Completer<number>()
        this.write_ = c
        this.writeData_ = s
        return c.promise
    }
    /**
     * 嘗試讀取數據，返回實際讀取的字節數
     */
    tryRead(s: Uint8Array | ArrayBuffer): number {
        if (this.closed_) {
            throw new NetError("TCPConn already closed")
        }
        if (!deps.get_length(s)) {
            return 0
        }
        return deps.tcp_read(this.conn_, s)
    }

    read(s: Uint8Array | ArrayBuffer): number | Promise<number> {
        if (this.closed_) {
            throw new NetError("TCPConn already closed")
        }
        if (!deps.get_length(s)) {
            return 0
        }
        const n = this.tryRead(s)
        if (n) {
            return n
        }
        return this._read(s)
    }
    private read_: _iotjs.Completer<number> | undefined
    private readData_: Uint8Array | ArrayBuffer | undefined
    private async _read(s: Uint8Array | ArrayBuffer): Promise<number> {
        // 等待未完成寫入
        let c = this.read_
        while (c) {
            await c
            if (this.closed_) {
                throw new NetError("TCPConn already closed")
            }
            c = this.write_
        }
        c = new _iotjs.Completer<number>()
        this.read_ = c
        this.readData_ = s
        return c.promise
    }

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
}
export interface WebsocketConnOptions {
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
        TCPConn.connect(opts.addr, opts.port, {
            tls: opts.wss,
            insecure: opts?.wss ? ws?.insecure ?? false : undefined,
            hostname: opts?.wss ? opts.sni : undefined,
            read: ws?.read,
            write: ws?.write,
        }, opts.hook).then(async (conn) => {
            this.conn_ = conn
            try {
                await conn.write(opts.handshake)
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
        }).catch((e) => {
            this._reject(e)
        })
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
    static connect(url: string, opts?: WebsocketConnOptions): Promise<WebsocketConn> {
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
        const handshake = `GET ${u.path} HTTP/1.1
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
        return new Promise((resolve, reject) => {
            connect.begin((conn, e) => {
                if (conn) {
                    let cc: WebsocketConn
                    try {
                        cc = new WebsocketConn(conn, hook)
                    } catch (e) {
                        reject(getError(e))
                        return
                    }
                    resolve(cc)
                } else {
                    reject(e)
                }
            })
        })
    }
    private constructor(private readonly conn_: TCPConn, hook: TCPConnHook) {
        hook.onRead = () => {
            this._onRead()
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
        return this.conn_.readable
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
    private _onRead() {
        const conn = this.conn_.conn_
        let readable = true
        // 讀取數據
        // const c = this.read_
        // if (c) {
        //     readable = false
        //     this.read_ = undefined
        //     const data = this.readData_!
        //     this.readData_ = undefined
        //     try {
        //         const n = deps.tcp_read(this.conn_, data)
        //         if (n) {
        //             c.resolve(n)
        //         } else {
        //             // 依然沒有數據，回調前可能有其它讀取
        //             this.read_ = undefined
        //             this.readData_ = data
        //         }
        //     } catch (e) {
        //         c.reject(getError(e))
        //     }
        // }

        // 通知上層用戶 有可讀數據
        const r = this.onReadable
        if (r) {
            if (!readable && !deps.ws_readable(conn)) {
                return;
            }
            readable = false;
            r()
        }

        // 通知上層用戶 有消息需要處理
        const m = this.onMessage
        if (m) {
            if (!readable && !deps.ws_readable(conn)) {
                return;
            }
            const data = deps.ws_read(conn)
            if (data) {
                m(data)
            }
            return
        }
    }
    // write(data: string | Uint8Array | ArrayBuffer): number | Promise<number>{}
    trySend(s: string | Uint8Array | ArrayBuffer): boolean {
        try {
            return deps.ws_send(this.conn_.conn_, s)
        } catch (e) {
            netError(e)
        }
    }
    // tryRead(s: string | Uint8Array | ArrayBuffer): number {
    // }
    // read(data: Uint8Array | ArrayBuffer): number | Promise<number>{
    // }
}
