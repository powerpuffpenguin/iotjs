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
    export function open(opts: OpenOptions, cb: (ret?: number, e?: any) => void): MTD
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

    export function seekSync(fd: MTD, offset: number, whence: number): number
    export function readSync(fd: MTD, data: Uint8Array): number
    export function writeSync(fd: MTD, data: Uint8Array): number
    export function seek(fd: MTD, offset: number, whence: number): number | undefined
    export function read(fd: MTD, data: Uint8Array): number | undefined
    export function write(fd: MTD, data: Uint8Array): number | undefined
}
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
export const Seek = deps.Seek
interface Task {
    evt: number
    cb?: (ret?: number, e?: any) => void

    offset?: number
    whence?: number

    data?: Uint8Array

    next?: Task
}
const EventSeek = 1
const EventRead = 2
const EventWrite = 3

export class File {
    private fd_: deps.MTD
    private close_ = false
    constructor(readonly path: string, write = false) {
        this.fd_ = deps.open({
            path: path,
            write: write,
        }, (ret, e) => runSync(() => {
            this._cb(ret, e)
        }))
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
    seekSync(offset: number, whence: deps.Seek) {
        if (this.close_) {
            throw new Error("db already closed")
        }
        return deps.seekSync(this.fd_, offset, whence)
    }
    readSync(data: Uint8Array): number {
        if (this.close_) {
            throw new Error("db already closed")
        }
        return deps.readSync(this.fd_, data)
    }
    writeSync(data: Uint8Array): number {
        if (this.close_) {
            throw new Error("db already closed")
        }
        return deps.writeSync(this.fd_, data)
    }
    private _cb(ret?: number, e?: any) {
        let front = this.front_
        if (!front) {
            throw new Error('mtd task empty');
        }
        switch (front.evt) {
            case EventSeek:
                break;
            case EventRead:
                break;
            case EventWrite:
                break;
            default:
                throw new Error(`unknow mtd evt ${front.evt}`);
        }
        const cb = front.cb
        if (cb) {
            cb(ret, e)
        }
        front = front.next
        if (front) {
            this.front_ = front
            this._pull(front)
        } else {
            this.front_ = undefined
            this.back_ = undefined
        }
    }

    private _pull(next?: Task) {
        let ret: number | undefined
        let cb: undefined | ((ret?: number, e?: any) => void)
        while (next) {
            if (this.close_) {
                const e = new Error("db already closed")
                while (next) {
                    cb = next.cb
                    if (cb) {
                        cb(undefined, e)
                    }
                    next = next.next
                }
                return
            }
            switch (next.evt) {
                case EventSeek:
                    ret = deps.seek(this.fd_, next.offset!, next.whence!)
                    break;
                case EventRead:
                    ret = deps.read(this.fd_, next.data!)
                    break;
                case EventWrite:
                    ret = deps.write(this.fd_, next.data!)
                    break;
                default:
                    throw new Error(`unknow mtd evt ${next.evt}`);
            }
            if (ret === undefined) {
                return
            }
            // 已經完成,調用回調
            cb = next.cb
            if (cb) {
                cb(ret)
            }
            // 執行下一任務
            next = next.next
            if (!next) {
                this.front_ = undefined
                this.back_ = undefined
                break
            }
            this.front_ = next
        }
    }
    private front_?: Task
    private back_?: Task
    private _push(next: Task): number | undefined {
        if (this.close_) {
            throw new Error("db already closed")
        }
        // 存在任務等待其它任務完成
        const back = this.back_
        if (back) {
            back.next = next
            this.back_ = next
            return
        }
        // 沒有其它任務，執行任務
        let ret: number | undefined
        switch (next.evt) {
            case EventSeek:
                ret = deps.seek(this.fd_, next.offset!, next.whence!)
                break;
            case EventRead:
                ret = deps.read(this.fd_, next.data!)
                break;
            case EventWrite:
                ret = deps.write(this.fd_, next.data!)
                break;
            default:
                throw new Error(`unknow mtd evt ${next.evt}`);
        }
        // 已經執行成功直接返回
        if (ret !== undefined) {
            return ret
        }

        // 等待任務完成
        this.back_ = next
        this.front_ = next

        return undefined
    }
    seek(offset: number, whence: deps.Seek, cb?: (ret?: number, e?: any) => void): number | undefined {
        return this._push({
            evt: EventSeek,
            offset: offset,
            whence: whence,
            cb: cb,
        })
    }
    read(data: Uint8Array, cb?: (ret?: number, e?: any) => void): number | undefined {
        return this._push({
            evt: EventRead,
            data: data,
            cb: cb,
        })
    }
    write(data: Uint8Array, cb?: (ret?: number, e?: any) => void): number | undefined {
        return this._push({
            evt: EventWrite,
            data: data,
            cb: cb,
        })
    }
}
// const keys = new Map<string, _DB>()
// class _DB {
//     constructor(readonly path: string, readonly fd: deps.MTD) { }
//     reference = 1
// }
// export class DB {
//     private db_: _DB
//     private close_ = 0
//     constructor(path: string) {
//         path = _iotjs.path.clean(path)
//         const found = keys.get(path)
//         if (found) {
//             found.reference++
//             this.db_ = found
//         } else {
//             const fd = deps.open(path, true)
//             try {
//                 const db = new _DB(path, fd)
//                 keys.set(path, db)
//                 this.db_ = db
//             } catch (e) {
//                 deps.close(fd)
//                 throw e
//             }
//         }
//     }
//     close() {
//         if (this.close_) {
//             return
//         }
//         this.close_ = 1
//         const db = this.db_
//         db.reference--
//         if (!db.reference) {
//             keys.delete(db.path)
//             deps.close(db.fd)
//         }
//     }
//     info() {
//         if (this.close_) {
//             throw new Error("db already closed")
//         }
//         return deps.info(this.db_.fd)
//     }
// }