

declare namespace iotjs {
    // 編譯目標 os
    export const os: string
    // 編譯目標 arch
    export const arch: string
}
const defaultUserAgent = `iotjs-${iotjs.os}-${iotjs.arch}-client/1.1`
declare namespace deps {
    export interface URL {
        scheme: string
        host: string
        port?: number
        userinfo?: string
        path: string
        query?: string
        fragment?: string
    }
    export function parse_url(url: string): URL
    export class Conn {
        __hash_http_conn: string
        onClose: () => void
    }
    export interface ConnectOptions {
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
         * 連接地址
         */
        addr: string
        /**
         * 連接端口
         */
        port: number
    }
    export function connect(opts: ConnectOptions): Conn
    export function close(conn: Conn): void
    export class Request {
        __hash_http_request: string
    }
    export function new_request(body: undefined | string | Uint8Array | ArrayBuffer, cb: (resp: Response, e: any) => void): Request
    export function free_request(req: Request): void
    export function add_header(req: Request, key: string, value: string): void
    export interface Response {
        code: number
        header: Record<string, string>
        body?: Uint8Array
    }
    export interface MakeRequestOptions {
        conn: Conn
        req: Request
        method: string
        path: string
        limit: number
    }
    export function make_request(opts: MakeRequestOptions): void
    export function cancel_request(req: Request): void
}

export class HTTPError extends _iotjs.IotError {
    constructor(message?: string | undefined, options?: ErrorOptions | undefined) {
        super(message, options)
        this.name = "NetError"
    }
    cancel?: boolean
}

function getError(e: any): any {
    if (typeof e === "string") {
        return new HTTPError(e)
    }
    return e
}
function runSync<T>(f: () => T, exit?: boolean) {
    try {
        return f()
    } catch (e) {
        e = getError(e)
        if (exit) {
            if (e instanceof Error) {
                console.error(e.toString())
            } else {
                console.error(e)
            }
            _iotjs.exit(1)
        } else {
            throw e
        }
    }
}

export const parseURL = deps.parse_url;
export interface ClientOptions {
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
}
export class Client {
    private conn_: deps.Conn
    private host_: string
    constructor(addr: string, port: number, opts?: ClientOptions) {
        const tls = opts?.tls ?? false
        this.conn_ = deps.connect({
            tls: tls,
            hostname: tls ? (opts?.hostname ?? addr) : undefined,
            insecure: tls ? (opts?.insecure ?? false) : undefined,
            addr: addr,
            port: port,
        })
        this.conn_.onClose = () => {
            runSync(() => {
                this.closed_ = -1
                deps.close(this.conn_)
                const f = this.onClose
                if (f) {
                    f()
                }
            }, true)
        }
        let host = opts?.hostname ?? addr
        if (tls) {
            if (port != 443) {
                host = `${host}:${port}`
            }
        } else {
            if (port != 80) {
                host = `${host}:${port}`
            }
        }
        this.host_ = host
    }
    private closed_ = 0
    get isClosed(): boolean {
        return this.closed_ ? true : false
    }
    close() {
        if (this.closed_) {
            return
        }
        this.closed_ = 1
        deps.close(this.conn_)
        const f = this.onClose
        if (f) {
            runSync(f, true)
        }
    }
    onClose?: () => boolean

    do(opts: RequestOptions, cb?: (resp?: deps.Response, e?: any) => void): Cancel {
        let body: undefined | string | Uint8Array | ArrayBuffer
        const method = opts.method ?? "GET"
        switch (method) {
            case "POST":
            case "PUT":
            case "PATCH":
                body = opts.body
            case "GET":
            case "HEAD":
            case "DELETE":
                break;
            default:
                throw new Error(`not supported method: ${method}`)
        }
        let ok = false
        const req = deps.new_request(body, (resp, e) => {
            if (ok) {
                return
            }
            ok = true
            if (cb) {
                runSync(() => {
                    if (resp) {
                        cb!(resp)
                    } else {
                        cb!(undefined, getError(e))
                    }
                })
            }
        })
        const cancel = new Cancel((e?: any) => {
            if (ok) {
                deps.cancel_request(req)
                return
            }
            ok = true
            if (cb) {
                runSync(() => {
                    if (e === undefined || e === null) {
                        e = new HTTPError("cancel")
                        e.cancel = true
                    }
                    cb!(undefined, e)
                })
            }
        })
        try {
            const header = opts.header
            let host = false
            let userAgent = false
            let accept = false
            if (header) {
                for (const key in header) {
                    if (Object.prototype.hasOwnProperty.call(header, key)) {
                        deps.add_header(req, key, header[key])
                        const lower = key.toLowerCase()
                        if (lower === "host") {
                            host = true
                        } else if (lower === "user-agent") {
                            userAgent = true
                        } else if (lower === "accept") {
                            accept = true
                        }
                    }
                }
            }
            if (!host) {
                deps.add_header(req, "Host", this.host_)
            }
            if (!userAgent) {
                deps.add_header(req, "User-Agent", defaultUserAgent)
            }
            if (!accept) {
                deps.add_header(req, "Accept", "*/*")
            }
        } catch (e) {
            deps.free_request(req)
            throw getError(e)
        }
        runSync(() => {
            deps.make_request({
                conn: this.conn_,
                req: req,
                method: method,
                path: opts.path ?? '/',
                limit: opts?.limit ?? 5 * 1024 * 1024,
            })
        })
        return cancel
    }
}
export class Cancel {
    constructor(readonly cancel?: (e?: any) => void) { }
}
export interface RequestOptions {
    limit?: number
    path?: string
    method?: 'GET' | 'HEAD' | 'POST' | 'PUT' | 'PATCH' | 'DELETE'
    header?: Record<string, string>
    body?: string | Uint8Array | ArrayBuffer
}
export class Response {
    constructor(public code: number,
        public header: Record<string, string>,
        public body?: Uint8Array,
    ) { }
}



