declare module "iotjs" {
    export const version: string


}
declare module "iotjs/fs" {
    export enum FileMode {
        dir = 0x80000000,// d: 是一個目錄
        // ModeAppend                                     // a: append-only
        // ModeExclusive                                  // l: exclusive use
        // ModeTemporary                                  // T: temporary file; Plan 9 only
        // ModeSymlink                                    // L: symbolic link
        // ModeDevice                                     // D: device file
        // ModeNamedPipe                                  // p: named pipe (FIFO)
        // ModeSocket                                     // S: Unix domain socket
        // ModeSetuid                                     // u: setuid
        // ModeSetgid                                     // g: setgid
        // ModeCharDevice                                 // c: Unix character device, when ModeDevice is set
        // ModeSticky                                     // t: sticky
        // ModeIrregular                                  // ?: non-regular file; nothing else is known about this file
    }
    export interface Fileinfo {
        // 檔案名稱
        readonly name: string
        // 常規檔案的大小
        readonly size: number | string
        // 返回 bits
        readonly mode: FileMode
        // 檔案修改時間
        readonly modTime: Date
        // 如果是目錄 返回 true 否則返回 false
        readonly isDir: boolean
    }
    export function stat(): Promise<Fileinfo>
}
declare var process: {
    argv: Array<string>
};