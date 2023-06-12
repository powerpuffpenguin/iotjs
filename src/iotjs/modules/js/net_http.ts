

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
    export class Response {
        code: number
        header: Record<string, string>
        body?: Uint8Array
    }
    export function make_request(conn: Conn, req: Request, method: string, path: string): void
}

export class HTTPError extends _iotjs.IotError {
    constructor(message?: string | undefined, options?: ErrorOptions | undefined) {
        super(message, options)
        this.name = "NetError"
    }
    eof?: boolean
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

export const parse_url = deps.parse_url;
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

    do(opts: RequestOptions, cb: (resp?: Response, e?: any) => void) {
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
        const req = deps.new_request(body, (resp, e) => {
            runSync(() => {
                if (resp) {
                    cb(resp)
                } else {
                    cb(undefined, getError(e))
                }
            })
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
            deps.make_request(this.conn_, req, method, opts.path ?? '/')
        })
    }
}

export interface RequestOptions {
    method?: 'GET' | 'HEAD' | 'POST' | 'PUT' | 'PATCH' | 'DELETE'
    path?: string
    header?: Record<string, string>
    body?: string | Uint8Array | ArrayBuffer
}
export class Response {
    constructor(public code: number,
        public header: Record<string, string>,
        public body?: Uint8Array,
    ) { }
}
// interface Conn {
//     conn: deps.Conn
//     tls: boolean
//     host: string
//     port: number
// }
// function safe_free_connect(conn: deps.Conn) {
//     try {
//         deps.free_connect(conn)
//     } catch (_) {

//     }
// }
// class Conns {
//     private arrs_: Array<Conn> = new Array()
//     connect(tls: boolean, host: string, port: number): Conn {
//         const arrs = this.arrs_
//         for (let i = arrs.length - 1; i >= 0; i--) {
//             if (deps.is_close_conn(arrs[i].conn)) {
//                 safe_free_connect(arrs[i].conn)
//                 arrs.splice(i, 1)
//             }
//         }
//         for (let i = 0; i < arrs.length; i++) {
//             const node = arrs[i];
//             if (node.tls === tls && node.port === port && node.host === host) {
//                 this.arrs_.splice(i, 1)
//                 return node
//             }
//         }
//         const ok = deps.connect(tls, host, port)
//         const conn = {
//             conn: ok,
//             tls: tls,
//             host: host,
//             port: port,
//         }
//         return conn
//     }
//     push(node: Conn) {
//         const arrs = this.arrs_
//         if (arrs.length >= 3) {
//             arrs.splice(0, 1)
//         }
//         arrs.push(node)
//     }
//     close() {
//         const arrs = this.arrs_
//         if (arrs.length) {
//             for (let i = 0; i < arrs.length; i++) {
//                 safe_free_connect(arrs[i].conn)
//             }
//             arrs.splice(0, arrs.length)
//         }
//     }
// }
// const defaultConns = new Conns()
// export async function request(url: string, opts?: RequestOptions): Promise<Response> {
//     const method = opts?.method ?? 'GET'
//     let body: undefined | string | Uint8Array | ArrayBuffer
//     switch (method) {
//         case "POST":
//         case "PUT":
//         case "PATCH":
//             body = opts?.body
//         case "GET":
//         case "HEAD":
//         case "DELETE":
//             break;
//         default:
//             throw new Error(`not supported method: ${method}`)
//     }
//     const uri = deps.uri_parse(url)
//     let port = uri.port
//     let tls: boolean
//     if (uri.scheme === "https") {
//         if (!port) {
//             port = 443
//         }
//         tls = true
//     } else if (uri.scheme === "http") {
//         if (!port) {
//             port = 80
//         }
//         tls = false
//     } else {
//         throw new Error(`not supported scheme: ${uri.scheme}`)
//     }
//     const conn = defaultConns.connect(tls, opts?.addr ?? uri.host, opts?.port ?? port)
//     let req: undefined | deps.Request
//     try {
//         req = deps.new_request(body)
//         const header = opts?.header
//         let host = false
//         let userAgent = false
//         let accept = false
//         if (header) {
//             for (const key in header) {
//                 if (Object.prototype.hasOwnProperty.call(header, key)) {
//                     deps.add_header(req, key, header[key])
//                     const lower = key.toLowerCase()
//                     if (lower === "host") {
//                         host = true
//                     } else if (lower === "user-agent") {
//                         userAgent = true
//                     } else if (lower === "accept") {
//                         accept = true
//                     }
//                 }
//             }
//         }
//         if (!host) {
//             deps.add_header(req, "Host", uri.port ? `${uri.host}:${uri.port}` : uri.host)
//         }
//         if (!userAgent) {
//             deps.add_header(req, "User-Agent", "iotjs")
//         }
//         if (!accept) {
//             deps.add_header(req, "Accept", "*/*")
//         }
//         const path = uri.query ? `${uri.path}?${uri.query}` : uri.path;

//         const resp = await deps.make_request(conn.conn, req, method, path)
//         defaultConns.push(conn)
//         return new Response(resp.code, resp.header, resp.body)
//     } catch (e) {
//         safe_free_connect(conn.conn)
//         throw e;
//     } finally {
//         if (req) {
//             try {
//                 deps.free_request(req)
//             } catch (e) { }
//         }
//     }
// }
// export function close_idle() {
//     defaultConns.close()
// }



