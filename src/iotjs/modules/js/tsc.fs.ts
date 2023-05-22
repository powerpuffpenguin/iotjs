declare namespace deps {
    export function open(name: string, flags: number, perm: number): Promise<cFile>
    export function openSync(name: string, flags: number, perm: number): cFile
}
export interface cFile { }
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
    static async open(name: string, flags = 0, perm = 0): Promise<File> {
        const c = await deps.open(name, flags, perm)
        return new File(c, flags & O_APPEND ? true : false)

    }
    static openSync(name: string, flags = 0, perm = 0): File {
        const c = deps.openSync(name, flags, perm)
        const file = new File(c, flags & O_APPEND ? true : false)
        return file
    }
    private constructor(private c_: cFile, private readonly append_: boolean) { }
}