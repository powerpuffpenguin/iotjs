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
    export class Request { }
    export function new_request(body?: string | Uint8Array | ArrayBuffer): Request
    export function add_header(req: Request, key: string, value: string): void
    export class Response { }
    export function make_request(conn: Conn, req: Request, method: string, path: string): Promise<Response>
}
export interface RequestOptions {
    method?: 'GET' | 'HEAD' | 'POST' | 'PUT' | 'PATCH' | 'DELETE'
    header?: Record<string, string>
    body?: string | Uint8Array | ArrayBuffer
}
export class Response {
    constructor(private readonly resp_: deps.Response) { }
}
interface Conn {
    conn: deps.Conn
    tls: boolean
    host: string
    port: number
}
class Conns {
    private arrs_: Array<Conn> = new Array()
    connect(tls: boolean, host: string, port: number): Conn {
        const arrs = this.arrs_
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
}
const defaultConns = new Conns()
export function request(url: string, opts?: RequestOptions): Promise<Response> {
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

    const req = deps.new_request(body)
    const header = opts?.header
    let host = false
    let userAgent = false
    if (header) {
        for (const key in header) {
            if (Object.prototype.hasOwnProperty.call(header, key)) {
                deps.add_header(req, key, header[key])
                const lower = key.toLowerCase()
                if (lower === "host") {
                    host = true
                } else if (lower === "user-agent") {
                    userAgent = true
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
    const path = uri.query ? `${uri.path}?${uri.query}` : uri.path;
    return deps.make_request(conn.conn, req, method, path).then(function (resp) {
        defaultConns.push(conn)
        return new Response(resp)
    })
}