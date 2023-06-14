import { Command } from "../flags";
import *as net from "iotjs/net";
import { go, YieldContext } from "iotjs/coroutine";
class Client {
    constructor(readonly conn: net.TCPConn, readonly echo: boolean, readonly recv: boolean) { }
    async send(co: YieldContext, i: number) {
        const at = this.echo ? Date.now() : undefined
        if (!this.recv) {
            this._write(co, `message ${i}`)
        }
        this._recv(co)
        if (at) {
            const used = Date.now() - at
            console.log(`used ${used / 1000}s`)
        }
    }
    private _co_write(co: YieldContext, s: string | Uint8Array): number {
        return co.yield((notify) => {
            const cb = (n?: number, e?: any) => {
                if (n === undefined) {
                    notify.error(e)
                } else {
                    notify.value(n)
                }
            }
            const n = this.conn.write(s, cb)
            if (typeof n === "number") {
                cb(n)
            }
        })
    }
    private _write(co: YieldContext, s: string) {
        const at = this.echo ? Date.now() : undefined
        const buf = this.buf
        new DataView(buf.buffer).setUint32(0, s.length)
        this._co_write(co, buf)
        this._co_write(co, s)
        if (at) {
            const used = Date.now() - at
            console.log(`send ${s}, used ${used / 1000}s`)
        }
    }
    private buf = new Uint8Array(4)
    private msg = new Uint8Array(1024)
    private _co_read(co: YieldContext, data: Uint8Array): number {
        return co.yield((notify) => {
            const cb = (n?: number, e?: any) => {
                if (n === undefined) {
                    notify.error(e)
                } else {
                    notify.value(n)
                }
            }
            const n = this.conn.read(data, cb)
            if (typeof n === "number") {
                cb(n)
            }
        })
    }
    private readfull(co: YieldContext, data: Uint8Array) {
        let n: number
        while (data.length) {
            n = this._co_read(co, data)
            data = data.subarray(n)
        }
    }
    private _recv(co: YieldContext) {
        const at = this.echo ? Date.now() : undefined
        const buf = this.buf
        this.readfull(co, buf)
        const n = new DataView(buf.buffer).getUint32(0)

        let s = ''
        if (n) {
            if (n > 1024) {
                throw new Error(`msg too large: ${n}`);
            }
            const buf = this.msg.subarray(0, n)

            this.readfull(co, buf)
            s = new TextDecoder().decode(buf)
        }
        if (at) {
            const used = Date.now() - at
            console.log(`recv ${s}, used ${used / 1000}s`)
        }
    }
}
export const command = new Command({
    use: 'co',
    short: 'test tcp echo client by coroutine',
    prepare: (flags) => {
        const recv = flags.bool({
            name: "recv",
            usage: "only recv not send",
        })
        const echo = flags.bool({
            name: "echo",
            usage: "echo every frame used",
        })
        const tls = flags.bool({
            name: "tls",
            usage: "connect use tls",
        })
        const addr = flags.string({
            name: "addr",
            short: "a",
            usage: "connect hostname",
            default: "192.168.251.50",
        })
        const port = flags.number({
            name: "port",
            short: "p",
            usage: "connect port",
            default: 12233,
        })
        const count = flags.number({
            name: "count",
            short: "c",
            usage: "message count",
            default: 1000,
        })
        return () => go((co) => {
            const at = Date.now()
            try {
                const conn = co.yield<net.TCPConn>((notify) => {
                    const useTLS = tls.value
                    net.TCPConn.connect(addr.value, port.value, {
                        tls: useTLS,
                        insecure: useTLS ? true : undefined,
                    }, (conn, e) => {
                        if (conn) {
                            notify.value(conn)
                        } else {
                            notify.error(e)
                        }
                    })
                })
                console.log(`connect: ${addr.value}:${port.value} success`)
                try {
                    const client = new Client(conn, echo.value, recv.value)
                    if (count.value < 1) {
                        let i = 0
                        while (true) {
                            client.send(co, i)
                            if (i == Number.MAX_SAFE_INTEGER) {
                                i = 0
                            } else {
                                i++
                            }
                        }
                    } else {
                        for (let i = 0; i < count.value; i++) {
                            client.send(co, i)
                        }
                    }
                } finally {
                    conn.close()
                }
            } catch (e) {
                console.log(`err: ${e}`)
            }
            const used = Date.now() - at
            console.log(`all success, used ${used / 1000}s`)
        })
    }
})