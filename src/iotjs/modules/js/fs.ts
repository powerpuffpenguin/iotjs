declare namespace deps {
    export function open(name: string, flags: number, perm: number): Promise<cFile>
    export function openSync(name: string, flags: number, perm: number): cFile
    export function write(file: cFile, s: string | Uint8Array | ArrayBuffer): number
    export function writeSync(file: cFile, s: string | Uint8Array | ArrayBuffer): number
}
export class NetError extends _iotjs.IotError {
    constructor(readonly code: number, message?: string | undefined, options?: ErrorOptions | undefined) {
        super(message, options)
        this.name = "NetError"
    }
}
function throwError(e: any): never {
    if (typeof e === "object" && typeof e.code == "number" && typeof e.message == "string") {
        throw new NetError(e.code, e.message);
    }
    throw e
}

function runSync<T>(f: () => T) {
    try {
        return f()
    } catch (e) {
        throwError(e)
    }
}
async function runAsync<T>(f: () => Promise<T>) {
    try {
        return await f()
    } catch (e) {
        throwError(e)
    }
}

export interface cFile {
    fd: number
}
/**
 * 以 read-only 模式打開檔案
 */
export const O_RDONLY = 0x0
/**
 * 以 write-only 模式打開檔案
 */
export const O_WRONLY = 0x1
/**
 * 以 read-write 模式打開檔案
 */
export const O_RDWR = 0x2
/**
   * 寫入時將數據添加到檔案末尾
   */
export const O_APPEND = 0x400
/**
 * 如果檔案不存在，則創建一個新檔案
 */
export const O_CREATE = 0x40
/**
 * 與 create 一起使用，檔案必須不存在
 */
export const O_EXCL = 0x80
/**
 * 打開同步 io
 */
export const O_SYNC = 0x101000
/**
 * 打開時截斷常規可寫檔案
 */
export const O_TRUNC = 0x200

export class File {
    static open(name: string, flags = O_RDONLY, perm = 0): Promise<File> {
        return runAsync(async () => {
            const c = await deps.open(name, flags, perm)
            return new File(c, flags & O_APPEND ? true : false)
        })
    }
    static openSync(name: string, flags = O_RDONLY, perm = 0): File {
        return runSync(() => {
            const c = deps.openSync(name, flags, perm)
            return new File(c, flags & O_APPEND ? true : false)
        })
    }
    static create(name: string, flags = O_RDWR | O_CREATE | O_TRUNC, perm = 0o666): Promise<File> {
        return runAsync(async () => {
            const c = await deps.open(name, flags, perm)
            return new File(c, flags & O_APPEND ? true : false)
        })
    }
    static createSync(name: string, flags = O_RDWR | O_CREATE | O_TRUNC, perm = 0o666): File {
        return runSync(() => {
            const c = deps.openSync(name, flags, perm)
            return new File(c, flags & O_APPEND ? true : false)
        })
    }
    private constructor(private c_: cFile, private readonly append_: boolean) { }
    write(s: string | ArrayBuffer | Uint8Array): Promise<number> {
        return runAsync(async () => {
            return await deps.write(this.c_, s)
        })
    }
    writeSync(s: string | ArrayBuffer | Uint8Array): number {
        return runSync(() => {
            return deps.writeSync(this.c_, s)
        })
    }
}