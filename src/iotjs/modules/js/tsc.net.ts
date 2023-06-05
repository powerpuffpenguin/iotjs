declare namespace deps {
    export class TCPConn {
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
    export function tcp_connect(opts: TCPConn): Promise<TCPConn>
    /**
     * 手動釋放 conn
     */
    export function tcp_free(conn: TCPConn): void
}
export const IPv4len = 4
export const IPv6len = 16
export class NetError extends _iotjs.IotError {
    constructor(message?: string | undefined, options?: ErrorOptions | undefined) {
        super(message, options)
        this.name = "NetError"
    }
}

function runSync<T>(f: () => T) {
    try {
        return f()
    } catch (e) {
        if (typeof e === "string") {
            throw new NetError(e)
        }
        throw e
    }
}
async function runAsync<T>(f: () => Promise<T>) {
    try {
        return await f()
    } catch (e) {
        if (typeof e === "string") {
            throw new NetError(e)
        }
        throw e
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
export class TCPConn {
    static connect(addr: string, port: number, opts?: TCPConnOptions): Promise<TCPConn> {
        return runAsync(async () => {
            const tls = opts?.tls ?? false;
            const conn = await deps.tcp_connect({
                addr: addr,
                port: port,
                tls: tls,
                hostname: tls ? (opts?.hostname ?? addr) : undefined,
                timeout: opts?.timeout ?? 0,
                read: opts?.read ?? 1024 * 1024,
                write: opts?.write ?? 1024 * 1024,
            })
            return new TCPConn(conn)
        })
    }
    private constructor(private readonly conn_: deps.TCPConn) {

    }
    private closed_ = 0;
    close(): void {
        if (!this.closed_) {
            deps.tcp_free(this.conn_)
            this.closed_ = 1
        }
    }

}