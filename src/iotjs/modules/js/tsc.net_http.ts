declare namespace deps {
    export interface URL {
        scheme: string
        host: string
        port?: number
        // userinfo?: string
        path: string
        query?: string
        // fragment?: string
    }
    export function uri_parse(url: string): URL
    export class Conn { }
    export function connect(tls: boolean, host: string, port: number): Conn
    export function free_connect(conn: Conn): void
    export function is_close_conn(conn: Conn): boolean
    export class Request { }
    export function new_request(body?: string | Uint8Array | ArrayBuffer): Request
    export function free_request(req: Request): void
    export function add_header(req: Request, key: string, value: string): void
    export class Response {
        code: number
        header: Record<string, string>
        body?: Uint8Array
    }
    export function make_request(conn: Conn, req: Request, method: string, path: string): Promise<Response>
}
export interface RequestOptions {
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
interface Conn {
    conn: deps.Conn
    tls: boolean
    host: string
    port: number
}
function safe_free_connect(conn: deps.Conn) {
    try {
        deps.free_connect(conn)
    } catch (_) {

    }
}
class Conns {
    private arrs_: Array<Conn> = new Array()
    connect(tls: boolean, host: string, port: number): Conn {
        const arrs = this.arrs_
        for (let i = arrs.length - 1; i >= 0; i--) {
            if (deps.is_close_conn(arrs[i].conn)) {
                safe_free_connect(arrs[i].conn)
                arrs.splice(i, 1)
            }
        }
        for (let i = 0; i < arrs.length; i++) {
            const node = arrs[i];
            if (node.tls === tls && node.port === port && node.host === host) {
                this.arrs_.splice(i, 1)
                return node
            }
        }
        const ok = deps.connect(tls, host, port)
        const conn = {
            conn: ok,
            tls: tls,
            host: host,
            port: port,
        }
        return conn
    }
    push(node: Conn) {
        const arrs = this.arrs_
        if (arrs.length >= 3) {
            arrs.splice(0, 1)
        }
        arrs.push(node)
    }
    close() {
        const arrs = this.arrs_
        if (arrs.length) {
            for (let i = 0; i < arrs.length; i++) {
                safe_free_connect(arrs[i].conn)
            }
            arrs.splice(0, arrs.length)
        }
    }
}
const defaultConns = new Conns()
export async function request(url: string, opts?: RequestOptions): Promise<Response> {
    const method = opts?.method ?? 'GET'
    let body: undefined | string | Uint8Array | ArrayBuffer
    switch (method) {
        case "POST":
        case "PUT":
        case "PATCH":
            body = opts?.body
        case "GET":
        case "HEAD":
        case "DELETE":
            break;
        default:
            throw new Error(`not supported method: ${method}`)
    }
    const uri = deps.uri_parse(url)
    let port = uri.port
    let tls: boolean
    if (uri.scheme === "https") {
        if (!port) {
            port = 443
        }
        tls = true
    } else if (uri.scheme === "http") {
        if (!port) {
            port = 80
        }
        tls = false
    } else {
        throw new Error(`not supported scheme: ${uri.scheme}`)
    }
    const conn = defaultConns.connect(tls, uri.host, port)
    let req: undefined | deps.Request
    try {
        req = deps.new_request(body)
        const header = opts?.header
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
            deps.add_header(req, "Host", uri.port ? `${uri.host}:${uri.port}` : uri.host)
        }
        if (!userAgent) {
            deps.add_header(req, "User-Agent", "iotjs")
        }
        if (!accept) {
            deps.add_header(req, "Accept", "*/*")
        }
        const path = uri.query ? `${uri.path}?${uri.query}` : uri.path;

        const resp = await deps.make_request(conn.conn, req, method, path)
        defaultConns.push(conn)
        return new Response(resp.code, resp.header, resp.body)
    } catch (e) {
        safe_free_connect(conn.conn)
        throw e;
    } finally {
        if (req) {
            try {
                deps.free_request(req)
            } catch (e) { }
        }
    }
}
export function close_idle() {
    defaultConns.close()
}