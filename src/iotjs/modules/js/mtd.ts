declare namespace deps {
    export enum Seek {
        set = 0,
        cur = 1,
        end = 2,
    }
    export interface MTD {
        readonly __iotjs_mtd_hash: string
    }
    export interface OpenOptions {
        path: string
        write: boolean
    }
    /**
     * 打開一個 mtd 分區
     * @param path 分區路徑
     * @param write 是否可寫，默認只是可讀
     * @param excl 是否獨佔打開
     */
    export function open(opts: OpenOptions): MTD
    /**
     * 關閉分區
     */
    export function close(fd: MTD): void
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

    export function seek(fd: MTD, offset: number, whence: number): number
    export function read(fd: MTD, data: Uint8Array): number
    export function write(fd: MTD, data: Uint8Array): number
}

export const Seek = deps.Seek
export class File {
    private fd_: deps.MTD
    private close_ = false
    constructor(readonly path: string, write = false) {
        this.fd_ = deps.open({
            path: path,
            write: write,
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
    }
    info() {
        if (this.close_) {
            throw new Error("db already closed")
        }
        return deps.info(this.fd_)
    }
    seek(offset: number, whence: deps.Seek) {
        if (this.close_) {
            throw new Error("db already closed")
        }
        return deps.seek(this.fd_, offset, whence)
    }
    read(data: Uint8Array): number {
        if (this.close_) {
            throw new Error("db already closed")
        }
        return deps.read(this.fd_, data)
    }
    write(data: Uint8Array): number {
        if (this.close_) {
            throw new Error("db already closed")
        }
        return deps.write(this.fd_, data)
    }
}
const keys = new Map<string, _DB>()
class _DB {
    constructor(readonly path: string, readonly fd: deps.MTD) {
        const info = deps.info(fd)
        const blocks = info.size / info.erasesize
        console.log(blocks)
    }
    reference = 1
}
export class DB {
    private db_: _DB
    private close_ = 0
    constructor(path: string) {
        path = _iotjs.path.clean(path)
        const found = keys.get(path)
        if (found) {
            found.reference++
            this.db_ = found
        } else {
            const fd = deps.open({
                path: path,
                write: true,
            })
            try {
                const db = new _DB(path, fd)
                keys.set(path, db)
                this.db_ = db
            } catch (e) {
                deps.close(fd)
                throw e
            }
        }
    }
    close() {
        if (this.close_) {
            return
        }
        this.close_ = 1
        const db = this.db_
        db.reference--
        if (!db.reference) {
            keys.delete(db.path)
            deps.close(db.fd)
        }
    }
    info() {
        if (this.close_) {
            throw new Error("db already closed")
        }
        return deps.info(this.db_.fd)
    }
}