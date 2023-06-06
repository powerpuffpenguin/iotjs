import { Command } from "../flags";
import *as net from "iotjs/net";
class Client {
    constructor(readonly conn: net.TCPConn) { }
    async send(i: number) {
        const at = Date.now()
        await this._write(`message ${i}`)
        await this._recv()
        const used = Date.now() - at
        console.log(`used ${used / 1000}s`)
    }
    private async _write(s: string) {
        const at = Date.now()
        const buf = this.buf
        new DataView(buf).setUint32(0, s.length)
        await this.conn.write(buf)
        await this.conn.write(s)
        const used = Date.now() - at
        console.log(`-> ${s}, used ${used / 1000}s`)
    }
    private buf = new ArrayBuffer(4)
    private async readfull(data: ArrayBuffer | Uint8Array) {
        let dst: Uint8Array
        if (data instanceof ArrayBuffer) {
            dst = new Uint8Array(data)
        } else {
            dst = data
        }
        let n: number
        while (dst.length) {
            n = await this.conn.read(dst)
            dst = dst.slice(n)
        }
    }
    private async _recv() {
        const at = Date.now()
        const buf = this.buf
        await this.readfull(buf)
        const n = new DataView(buf).getUint32(0)

        const used = Date.now() - at
        console.log(`-> ${n}, used ${used / 1000}s`)
    }
}
export const command = new Command({
    use: 'tcp',
    short: 'test tcp echo client',
    prepare: (flags) => {
        const addr = flags.string({
            name: "addr",
            short: "a",
            usage: "connect hostname"
        })
        const port = flags.number({
            name: "port",
            short: "p",
            usage: "connect port",
        })
        const count = flags.number({
            name: "count",
            short: "c",
            usage: "message count",
            default: 10000 * 100,
        })
        return async () => {
            const at = Date.now()
            try {
                const conn = await net.TCPConn.connect(addr.value, port.value)
                console.log(`connect: ${addr.value}:${port.value} success`)
                try {
                    const client = new Client(conn)
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
            console.log(`used ${used / 1000}s`)
        }
    }
})