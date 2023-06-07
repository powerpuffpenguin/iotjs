import { Command } from "../flags";
import *as net from "iotjs/net";
class Client {
    constructor(readonly conn: net.TCPConn, readonly echo: boolean, readonly recv: boolean) { }
    async send(i: number) {
        const at = this.echo ? Date.now() : undefined
        if (!this.recv) {
            await this._write(`message ${i}`)
        }
        await this._recv()
        if (at) {
            const used = Date.now() - at
            console.log(`used ${used / 1000}s`)
        }
    }
    private async _write(s: string) {
        const at = this.echo ? Date.now() : undefined
        const buf = this.buf
        new DataView(buf.buffer).setUint32(0, s.length)
        await this.conn.write(buf)
        await this.conn.write(s)
        if (at) {
            const used = Date.now() - at
            console.log(`send ${s}, used ${used / 1000}s`)
        }
    }
    private buf = new Uint8Array(4)
    private msg = new Uint8Array(1024)
    private async readfull(data: Uint8Array) {
        let n: number
        while (data.length) {
            n = await this.conn.read(data)
            data = data.subarray(n)
        }
    }
    private async _recv() {
        const at = this.echo ? Date.now() : undefined
        const buf = this.buf
        await this.readfull(buf)
        const n = new DataView(buf.buffer).getUint32(0)

        let s = ''
        if (n) {
            if (n > 1024) {
                throw new Error(`msg too large: ${n}`);
            }
            const buf = this.msg.subarray(0, n)

            await this.readfull(buf)
            s = new TextDecoder().decode(buf)
        }
        if (at) {
            const used = Date.now() - at
            console.log(`recv ${s}, used ${used / 1000}s`)
        }
    }
}
export const command = new Command({
    use: 'tcp',
    short: 'test tcp echo client',
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
        return async () => {
            const at = Date.now()
            try {
                const conn = await net.TCPConn.connect(addr.value, port.value, {
                    tls: tls.value,
                })
                console.log(`connect: ${addr.value}:${port.value} success`)
                try {
                    const client = new Client(conn, echo.value, recv.value)
                    if (count.value < 1) {
                        let i = 0
                        while (true) {
                            await client.send(i)
                            if (i == Number.MAX_SAFE_INTEGER) {
                                i = 0
                            } else {
                                i++
                            }
                        }
                    } else {
                        for (let i = 0; i < count.value; i++) {
                            await client.send(i)
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
        }
    }
})