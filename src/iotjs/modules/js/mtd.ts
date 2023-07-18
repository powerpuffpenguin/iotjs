declare namespace deps {
    export enum Seek {
        set = 0,
        cur = 1,
        end = 2,
    }
    export interface MTD {
        readonly __iotjs_mtd_hash: string
    }
    /**
     * 打開一個 mtd 分區
     * @param path 分區路徑
     * @param write 是否可寫，默認只是可讀
     * @param excl 是否獨佔打開
     */
    export function open(path: string, cb: (evt: number, ret: number, e?: any) => void): MTD
    /**
     * 關閉分區
     */
    export function close(fd: MTD): void
    export function free(fd: MTD): void
    export interface Info {
        type: number
        flags: number
        size: number
        erasesize: number
        writesize: number
        oobsize: number
    }
    /**
     * 返回分區信息
     * @param fd 
     */
    export function info(fd: MTD): Info

    export function seekSync(fd: MTD, offset: number, whence: number): number
    export function eraseSync(fd: MTD, offset: number, size: number): void
    export function readSync(fd: MTD, data: Uint8Array): number
    export function writeSync(fd: MTD, data: Uint8Array): number

    export function seek(fd: MTD, offset: number, whence: number): void
    export function erase(fd: MTD, offset: number, size: number): void
    export function read(fd: MTD, data: Uint8Array): number | undefined
    export function write(fd: MTD, data: Uint8Array): number | undefined

    export class DB {
        readonly __iotjs_mtd_db_hash: string
    }
    export function db(path: string, device: number, cb: (evt: number, ret?: any, e?: any) => void): DB
    export function len(data: Uint8Array | ArrayBuffer | string): number
    /**
     * 關閉分區
     */
    export function db_close(db: DB): void
    export function db_free(db: DB): void
    export function key_encode(v: any): string
    export function db_set_sync(db: DB, k0: string, k1: string, data: Uint8Array | ArrayBuffer | string, buf: Uint8Array): void
    export function db_get_sync(db: DB, k0: string, k1: string, s: boolean): Uint8Array | string | undefined
    export function db_has_sync(db: DB, k0: string, k1: string): boolean
    export function db_delete_sync(db: DB, k0: string, k1: string): void
    export function db_info(db: DB): any

    export function db_set(db: DB, k0: string, k1: string, data: Uint8Array | ArrayBuffer | string, buf: Uint8Array): void
    export function db_get(db: DB, k0: string, k1: string, s: boolean): void
    export function db_has(db: DB, k0: string, k1: string): void
    export function db_delete(db: DB, k0: string, k1: string): void
}

export const Seek = deps.Seek
function runSync<T>(f: () => T, exit?: boolean) {
    try {
        return f()
    } catch (e) {
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
interface Task {
    next?: Task
    evt: number
    data?: Uint8Array
    offset?: number
    whence?: number
    size?: number
    cb?: (ret?: number, e?: any) => void
}
export class File {
    private fd_: deps.MTD
    private close_ = false
    private free_ = false
    constructor(readonly path: string) {
        this.fd_ = deps.open(path, (evt, ret, e) => {
            this._onEvt(evt, ret, e)
        })
    }
    private _onEvt(evt: number, ret?: number, e?: any) {
        runSync(() => {
            const front = this.front_!
            let next = front.next
            if (next) {
                this.front_ = next
            } else {
                this.front_ = undefined
                this.back_ = undefined
            }
            const cb = front.cb
            if (cb) {
                switch (evt) {
                    case 1: //erase
                        cb(e)
                        break
                    case 0: // seek
                    case 2: // read
                    case 3: // write
                        cb(ret, e)
                        break
                }
            }
            if (this.close_) {
                this._free()
                this.front_ = undefined
                this.back_ = undefined
                if (next) {
                    let e = new Error("mtd already closed")
                    while (next) {
                        if (next.cb) {
                            if (next.evt == 1) {
                                next.cb(e as any)
                            } else {
                                next.cb(undefined, e)
                            }
                        }
                        next = next.next
                    }
                }
                return
            }
            if (next) {
                ret = this._do(next)
                if (ret !== undefined) {
                    this._onEvt(next.evt, ret)
                }
            }
        })
    }
    get isClosed(): boolean {
        return this.close_
    }
    close() {
        if (this.close_) {
            return
        }
        this.close_ = true
        deps.close(this.fd_)
        if (!this.front_) {
            this._free()
        }
    }
    private _free() {
        if (!this.free_) {
            this.free_ = true
            deps.free(this.fd_)
        }
    }
    info() {
        if (this.close_) {
            throw new Error("mtd already closed")
        }
        return deps.info(this.fd_)
    }
    seekSync(offset: number, whence: deps.Seek) {
        if (this.close_) {
            throw new Error("mtd already closed")
        } else if (this.front_) {
            throw new Error("mtd busy")
        }
        return deps.seekSync(this.fd_, offset, whence)
    }
    eraseSync(offset: number, size: number): void {
        if (this.close_) {
            throw new Error("mtd already closed")
        } else if (this.front_) {
            throw new Error("mtd busy")
        }
        return deps.eraseSync(this.fd_, offset, size)
    }
    readSync(data: Uint8Array): number {
        if (this.close_) {
            throw new Error("mtd already closed")
        } else if (this.front_) {
            throw new Error("mtd busy")
        }
        return deps.readSync(this.fd_, data)
    }
    writeSync(data: Uint8Array): number {
        if (this.close_) {
            throw new Error("mtd already closed")
        } else if (this.front_) {
            throw new Error("mtd busy")
        }
        return deps.writeSync(this.fd_, data)
    }
    private front_?: Task
    private back_?: Task
    private _do(next: Task): number | undefined {
        let ret: number | undefined
        switch (next.evt) {
            case 0:
                try {
                    deps.seek(this.fd_, next.offset!, next.whence!)
                } catch (e) {
                    this._onEvt(0, undefined, e)
                }
                return
            case 1:
                try {
                    deps.erase(this.fd_, next.offset!, next.size!)
                } catch (e) {
                    this._onEvt(1, e as any)
                }
                return
            case 2:
                try {
                    ret = deps.read(this.fd_, next.data!)
                } catch (e) {
                    this._onEvt(2, undefined, e)
                    return
                }
                break
            case 3:
                try {
                    ret = deps.write(this.fd_, next.data!)
                } catch (e) {
                    this._onEvt(3, undefined, e)
                    return
                }
                break;
            default:
                throw new Error(`unknow task ${next.evt}`);
        }
        return ret
    }
    private _task(next: Task) {
        if (this.back_) {
            this.back_.next = next
            this.back_ = next
        } else {
            this.back_ = next
            this.front_ = next
            const ret = this._do(next)
            if (ret !== undefined) {
                this._onEvt(next.evt, ret)
            }
        }
    }
    seek(offset: number, whence: number, cb?: (ret?: number, e?: any) => void): void {
        if (this.close_) {
            throw new Error("mtd already closed")
        }
        this._task({ evt: 0, offset: offset, whence: whence, cb: cb })
    }
    erase(offset: number, size: number, cb?: (e?: any) => void): void {
        if (this.close_) {
            throw new Error("mtd already closed")
        }
        this._task({ evt: 1, offset: offset, size: size, cb: cb })
    }
    read(data: Uint8Array, cb?: (ret?: number, e?: any) => void): void {
        if (this.close_) {
            throw new Error("mtd already closed")
        }
        this._task({ evt: 2, data: data, cb: cb })
    }
    write(data: Uint8Array, cb?: (ret?: number, e?: any) => void): void {
        if (this.close_) {
            throw new Error("mtd already closed")
        }
        this._task({ evt: 3, data: data, cb: cb })
    }
}
const dbDevices = [false, false, false]

interface DBTask {
    next?: DBTask
    evt: 0 | 1 | 2 | 3
    k0: string
    k1: string

    s?: boolean

    data?: Uint8Array | ArrayBuffer | string
    buf?: Uint8Array

    cb?: (ret?: any, e?: any) => void
}
export class DB {
    private device_: number
    private db_: deps.DB
    private close_ = false
    constructor(readonly path: string) {
        for (let i = 0; i < dbDevices.length; i++) {
            if (!dbDevices[i]) {
                this.db_ = deps.db(path, i, (evt, ret, e) => {
                    this._onEvt(evt, ret, e)
                })
                dbDevices[i] = true
                this.device_ = i
                return
            }
        }
        throw new Error("device busy")
    }
    private _onEvt(evt: number, ret?: number, e?: any) {
        runSync(() => {
            const front = this.front_!
            let next = front.next
            if (next) {
                this.front_ = next
            } else {
                this.front_ = undefined
                this.back_ = undefined
            }
            const cb = front.cb
            if (cb) {
                switch (evt) {
                    case 0: // set
                    case 3: // delete
                        cb(e)
                        break
                    case 1: // has
                    case 2: // get
                        cb(ret, e)
                        break
                }
            }
            if (this.close_) {
                this._free()
                this.front_ = undefined
                this.back_ = undefined
                if (next) {
                    let e = new Error("db already closed")
                    while (next) {
                        if (next.cb) {
                            if (next.evt == 0) {
                                next.cb(e as any)
                            } else {
                                next.cb(undefined, e)
                            }
                        }
                        next = next.next
                    }
                }
                return
            }
            if (next) {
                try {
                    this._do(next)
                } catch (e) {
                    this._onEvt(next.evt, undefined, ret)
                }
            }
        })
    }
    get isClosed(): boolean {
        return this.close_
    }
    close() {
        if (this.close_) {
            return
        }
        this.close_ = true
        deps.db_close(this.db_)
        if (!this.front_) {
            this._free()
        }
    }
    private _free() {
        const i = this.device_
        if (i > -1) {
            this.device_ = -1
            dbDevices[i] = false
            deps.db_free(this.db_)
        }
    }
    setSync(key: string, data: Uint8Array | ArrayBuffer | string): void {
        if (this.close_) {
            throw new Error("db already closed")
        } else if (this.front_) {
            throw new Error("db busy")
        }
        const len = deps.len(data)
        if (len > 4294967295 - 4 - 8) {
            throw new Error("data too long")
        }
        const buf = new Uint8Array(len + 4 + 8) // len:4 + data + version:8
        key = deps.key_encode(key)
        const k0 = `/0.${key}`
        const k1 = `/1.${key}`
        deps.db_set_sync(this.db_, k0, k1, data, buf)
    }
    private _getSync(key: string, s: boolean) {
        if (this.close_) {
            throw new Error("db already closed")
        } else if (this.front_) {
            throw new Error("db busy")
        }
        key = deps.key_encode(key)
        const k0 = `/0.${key}`
        const k1 = `/1.${key}`
        return deps.db_get_sync(this.db_, k0, k1, s)
    }
    getSync(key: string) {
        return this._getSync(key, false)
    }
    getStringSync(key: string) {
        return this._getSync(key, true)
    }
    hasSync(key: string): boolean {
        if (this.close_) {
            throw new Error("db already closed")
        } else if (this.front_) {
            throw new Error("db busy")
        }
        key = deps.key_encode(key)
        const k0 = `/0.${key}`
        const k1 = `/1.${key}`
        return deps.db_has_sync(this.db_, k0, k1)
    }
    deleteSync(key: string): void {
        if (this.close_) {
            throw new Error("db already closed")
        } else if (this.front_) {
            throw new Error("db busy")
        }
        key = deps.key_encode(key)
        const k0 = `/0.${key}`
        const k1 = `/1.${key}`
        deps.db_delete_sync(this.db_, k0, k1)
    }
    info() {
        if (this.close_) {
            throw new Error("db already closed")
        }
        return deps.db_info(this.db_)
    }

    private front_?: DBTask
    private back_?: DBTask
    private _do(next: DBTask) {
        switch (next.evt) {
            case 0:// set
                deps.db_set(this.db_, next.k0, next.k1, next.data!, next.buf!)
                break
            case 1:// has
                deps.db_has(this.db_, next.k0, next.k1)
                break
            case 2: // get
                deps.db_get(this.db_, next.k0, next.k1, next.s!)
                break
            case 3:// delete
                deps.db_delete(this.db_, next.k0, next.k1)
                break
            default:
                throw new Error(`unknow task ${next.evt}`);
        }
    }
    private _task(next: DBTask) {
        if (this.back_) {
            this.back_.next = next
            this.back_ = next
        } else {
            this.back_ = next
            this.front_ = next
            try {
                this._do(next)
            } catch (e) {
                this._onEvt(next.evt, undefined, e)
            }
        }
    }
    set(key: string, data: Uint8Array | ArrayBuffer, cb?: (e?: any) => void): void {
        if (this.close_) {
            throw new Error("db already closed")
        }
        const len = deps.len(data)
        if (len > 4294967295 - 4 - 8) {
            throw new Error("data too long")
        }
        const buf = new Uint8Array(len + 4 + 8) // len:4 + data + version:8
        key = deps.key_encode(key)
        const k0 = `/0.${key}`
        const k1 = `/1.${key}`

        this._task({ evt: 0, k0: k0, k1: k1, data: data, buf: buf, cb: cb })
    }
    has(key: string, cb?: (exists?: boolean, e?: any) => void): void {
        if (this.close_) {
            throw new Error("db already closed")
        }
        key = deps.key_encode(key)
        const k0 = `/0.${key}`
        const k1 = `/1.${key}`

        this._task({ evt: 1, k0: k0, k1: k1, cb: cb })
    }
    private _get(key: string, s: boolean, cb?: (data?: Uint8Array, e?: any) => void) {
        if (this.close_) {
            throw new Error("db already closed")
        }
        key = deps.key_encode(key)
        const k0 = `/0.${key}`
        const k1 = `/1.${key}`

        this._task({ evt: 2, k0: k0, k1: k1, s: s, cb: cb })
    }
    get(key: string, cb?: (data?: Uint8Array, e?: any) => void): void {
        this._get(key, false, cb)
    }
    getString(key: string, cb?: (data?: Uint8Array, e?: any) => void): void {
        this._get(key, true, cb)
    }
    delete(key: string, cb?: (e?: any) => void): void {
        if (this.close_) {
            throw new Error("db already closed")
        }
        key = deps.key_encode(key)
        const k0 = `/0.${key}`
        const k1 = `/1.${key}`

        this._task({ evt: 3, k0: k0, k1: k1, cb: cb })
    }
}