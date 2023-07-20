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
    /**
     * 關閉分區
     */
    export function db_close(db: DB): void
    export function db_free(db: DB): void
    export function key_encode(v: any): string
    export function key_decode(v: string): string
    export function db_set_sync(db: DB, k0: string, k1: string, data: Uint8Array | ArrayBuffer | string, buf: Uint8Array): void
    export function db_get_sync(db: DB, k0: string, k1: string): Uint8Array | string | undefined
    export function db_has_sync(db: DB, k0: string, k1: string): boolean
    export function db_delete_sync(db: DB, k0: string, k1: string): void
    export function db_info(db: DB): any

    export function db_set(db: DB, k0: string, k1: string, data: Uint8Array | ArrayBuffer | string, buf: Uint8Array): void
    export function db_get(db: DB, k0: string, k1: string): void
    export function db_has(db: DB, k0: string, k1: string): void
    export function db_delete(db: DB, k0: string, k1: string): void

    export class DBIterator {
        readonly __iotjs_mtd_db_iterator_hash: string
    }
    export function db_iterator(db: DB): DBIterator
    export function db_iterator_free(iter: DBIterator): void
    export function db_iterator_foreach_sync(iter: DBIterator, data: boolean, cb: (key: string, high: number, low: number, data?: Uint8Array) => boolean): boolean
    export function db_iterator_foreach(iter: DBIterator, data: boolean): boolean
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
    /**
     * * 0 set
     * * 1 has
     * * 2 get
     * * 3 delete
     * * 4 foreach
     */
    evt: 0 | 1 | 2 | 3 | 4
    k0?: string
    k1?: string

    data?: Uint8Array | ArrayBuffer | string
    buf?: Uint8Array

    cb?: Function

    iter?: deps.DBIterator
    cp?: boolean
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
                    case 4: // foreach
                        if (!cb(ret, e)) { // 還需要繼續調用
                            // 放到隊列尾端
                            if (next) {
                                front.next = undefined
                                this.back_!.next = front
                            } else {
                                next = front
                                this.back_ = next
                                this.front_ = next
                            }
                        }
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
    setSync(key: string, data: Uint8Array | ArrayBuffer): void {
        if (this.close_) {
            throw new Error("db already closed")
        } else if (this.front_) {
            throw new Error("db busy")
        }
        const len = data.byteLength
        if (len > 2147483647 - 4 - 8) {
            throw new Error("data too long")
        }
        const buf = new Uint8Array(len + 4 + 8) // len:4 + data + version:8
        key = deps.key_encode(key)
        const k0 = `/0.${key}`
        const k1 = `/1.${key}`
        deps.db_set_sync(this.db_, k0, k1, data, buf)
    }
    getSync(key: string) {
        if (this.close_) {
            throw new Error("db already closed")
        } else if (this.front_) {
            throw new Error("db busy")
        }
        key = deps.key_encode(key)
        const k0 = `/0.${key}`
        const k1 = `/1.${key}`
        return deps.db_get_sync(this.db_, k0, k1)
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
    private _foreachSync(cp: boolean, cb: Function) {
        if (this.close_) {
            throw new Error("db already closed")
        } else if (this.front_) {
            throw new Error("db busy")
        }
        const iter = deps.db_iterator(this.db_)
        const record: Record<string, {
            ok?: boolean
            v?: { high: number, low: number, value?: Uint8Array }
        }> = {}
        try {
            const found = deps.db_iterator_foreach_sync(iter, cp, (key, high, low, value) => {
                const name = deps.key_decode(key.substring(3))
                const o = record[name]
                if (o) {
                    if (o.ok) {
                        return false
                    }
                    o.ok = true
                    const v = (high > o.v!.high) || (high == o.v!.high && low > o.v!.low) ? value : o.v!.value
                    o.v = undefined
                    return cb(name, v!) ? true : false
                } else {
                    if (cp) {
                        record[name] = {
                            v: { high: high, low: low, value: value }
                        }
                    } else {
                        record[name] = {
                            ok: true,
                        }
                        return cb(name) ? true : false
                    }
                }
                return false
            })
            if (found) {
                return
            }
            for (const name in record) {
                const o = record[name]
                if (o.ok) {
                    continue
                }
                const found = cp ? cb(name, o.v!.value) : cb(name)
                if (found) {
                    break
                }
            }
        } finally {
            deps.db_iterator_free(iter)
        }
    }
    keysSync(cb: (key: string) => boolean) {
        this._foreachSync(false, cb)
    }
    foreachSync(cb: (key: string, data: Uint8Array) => boolean): void {
        this._foreachSync(true, cb)
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
                deps.db_set(this.db_, next.k0!, next.k1!, next.data!, next.buf!)
                break
            case 1:// has
                deps.db_has(this.db_, next.k0!, next.k1!)
                break
            case 2: // get
                deps.db_get(this.db_, next.k0!, next.k1!)
                break
            case 3:// delete
                deps.db_delete(this.db_, next.k0!, next.k1!)
                break
            case 4: // foreach
                deps.db_iterator_foreach(next.iter!, next.cp!)
                break
            default:
                throw new Error(`unknow task ${next.evt}`)
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
        const len = data.byteLength
        if (len > 2147483647 - 4 - 8) {
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
    get(key: string, cb?: (data?: Uint8Array, e?: any) => void): void {
        if (this.close_) {
            throw new Error("db already closed")
        }
        key = deps.key_encode(key)
        const k0 = `/0.${key}`
        const k1 = `/1.${key}`

        this._task({ evt: 2, k0: k0, k1: k1, cb: cb })
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
    private _foreach(cp: boolean, cb: Function) {
        if (this.close_) {
            throw new Error("db already closed")
        }
        const iter = deps.db_iterator(this.db_)
        const record: Record<string, {
            ok?: boolean
            v?: { high: number, low: number, value?: Uint8Array }
        }> = {}
        this._task({
            evt: 4,
            iter: iter,
            cp: cp,
            cb: (ret?: {
                finish: boolean,
                key?: string,
                high?: number,
                low?: number,
                value?: Uint8Array
            }, e?: any) => {
                if (ret) {
                    if (ret.finish) {
                        for (const name in record) {
                            const o = record[name]
                            if (o.ok) {
                                continue
                            }
                            const found = cp ? cb(false, name, o.v!.value) : cb(false, name)
                            if (found) {
                                deps.db_iterator_free(iter)
                                return
                            }
                        }
                        deps.db_iterator_free(iter)
                        cb(true)
                    } else {
                        const name = deps.key_decode(ret.key!.substring(3))
                        const o = record[name]
                        if (o) {
                            if (o.ok) {
                                return false
                            }
                            o.ok = true
                            const v = (ret.high! > o.v!.high) || (ret.high! == o.v!.high && ret.low! > o.v!.low) ? ret.value : o.v!.value
                            o.v = undefined
                            const found = cb(false, name, v!) ? true : false
                            if (found) {
                                deps.db_iterator_free(iter)
                            }
                            return found
                        } else {
                            if (cp) {
                                record[name] = {
                                    v: { high: ret.high!, low: ret.low!, value: ret.value }
                                }
                            } else {
                                record[name] = {
                                    ok: true,
                                }
                                const found = cb(false, name) ? true : false
                                if (found) {
                                    deps.db_iterator_free(iter)
                                }
                                return found
                            }
                        }
                        return false
                    }
                } else {
                    deps.db_iterator_free(iter)
                    if (cp) {
                        cb(undefined, undefined, undefined, e)
                    } else {
                        cb(undefined, undefined, e)
                    }
                }
            },
        })
    }
    keys(cb: (finish?: boolean, key?: string, e?: any) => boolean): void {
        this._foreach(false, cb)
    }
    foreach(cb: (finish?: boolean, key?: string, data?: Uint8Array, e?: any) => boolean): void {
        this._foreach(true, cb)
    }
}