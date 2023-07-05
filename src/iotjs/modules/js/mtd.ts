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

    export function seek(fd: MTD, offset: number, whence: number): number
    export function read(fd: MTD, data: Uint8Array): number
    export function write(fd: MTD, data: Uint8Array): number
}

export const Seek = deps.Seek

interface Task {
    next?: Task
    evt: number
    data?: Uint8Array
    offset?: number
    whence?: number
    cb?: (ret?: number, e?: any) => void
}
export class File {
    private fd_: deps.MTD
    private close_ = false
    constructor(readonly path: string) {
        this.fd_ = deps.open(path, (evt, errno, ret) => {
            console.log(evt, errno, ret)
            switch (evt) {
                case 0: // seek

                    break;
                case 1: // read

                    break
                case 2: // write

                    break
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
    private front_?: Task
    private back_?: Task
    private _do(next: Task) {
        switch (next.evt) {
            case 0:
                deps.seek(this.fd_, next.offset!, next.whence!)
                break;
            case 1:
                deps.read(this.fd_, next.data!)
                break;
            case 2:
                deps.write(this.fd_, next.data!)
                break;
            default:
                throw new Error(`unknow task ${next.evt}`);
        }
    }
    private _task(next: Task) {
        if (this.back_) {
            this.back_.next = next
        } else {
            this._do(next)
            this.back_ = next
            this.front_ = next
        }
    }
    seek(offset: number, whence: number, cb?: (ret?: number, e?: any) => void): void {
        if (this.close_) {
            throw new Error("db already closed")
        }
        this._task({ evt: 0, offset: offset, whence: whence, cb: cb })
    }
    read(data: Uint8Array, cb?: (ret?: number, e?: any) => void): void {
        if (this.close_) {
            throw new Error("db already closed")
        }
        this._task({ evt: 1, data: data, cb: cb })
    }
    write(data: Uint8Array, cb?: (ret?: number, e?: any) => void): void {
        if (this.close_) {
            throw new Error("db already closed")
        }
        this._task({ evt: 2, data: data, cb: cb })
    }
}
