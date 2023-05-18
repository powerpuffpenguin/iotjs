namespace iotjs {
    export * from "iotjs"
}

declare module "iotjs" {
    export const version: string
    export const argv: Array<string>
    export class Completer<T> {
        readonly isCompleted: boolean
        readonly promise: Promise<T>
        resolve(v: T): void
        reject(reason?: any): void
    }
    export class IotError extends Error {
        code: number
    }
}
declare module "iotjs/fs" {
    export enum FileMode {
        dir = 0x80000000,// d: 是一個目錄
        append = 0x40000000,                               // a: append-only
        exclusive = 0x20000000,                               // l: exclusive use
        temporary = 0x10000000,                                 // T: temporary file; Plan 9 only
        link = 0x8000000,                                  // L: symbolic link
        device = 0x4000000,                         // D: device file
        pipe = 0x2000000,                                  // p: named pipe (FIFO)
        socket = 0x1000000,                                  // S: Unix domain socket
        setuid = 0x800000,                                    // u: setuid
        setgid = 0x400000,                                     // g: setgid
        charDevice = 0x200000,                                 // c: Unix character device, when ModeDevice is set
        sticky = 0x100000,                                    // t: sticky
        irregular = 0x80000,                                  // ?: non-regular file; nothing else is known about this file

        /**
         * Mask for the type bits. For regular files, none will be set.
         * 
         *  (dir | link | pipe | socket | device | charDevice | irregular)
         */
        type = 0x8f280000,

        perm = 0o777, // Unix permission bits
    }
    export interface Fileinfo {
        // 檔案名稱
        readonly name: string
        // 常規檔案的大小
        readonly size: number | string
        // 返回 bits
        readonly mode: FileMode
        // 檔案修改時間
        readonly mtime: Date
        // 如果是目錄 返回 true 否則返回 false
        readonly isDir: boolean
    }
    export function stat(path: string): Promise<Fileinfo>
    export function statSync(path: string): Fileinfo
}
declare module "iotjs/net" {
    /**
     * 表示一個網路端點的地址
     */
    export interface Addr {
        /**
         * 使用的網路名稱，例如 "tcp" "udp"
         */
        network: string
        // 網路地址，例如 "192.0.2.1:25", "[2001:db8::1]:80"
        address: string
    }

    /**
     * 抽象的網路流連接，可以在其上進行雙向的數據量傳輸
     */
    export interface Conn {
        read(buffer: Uint8Array): Promise<number>
        write(buffer: Uint8Array): Promise<number>
        close(): Promise<void>
        localAddr(): Addr
        remoteAddr(): Addr
        setDeadline(t: Date): void
        setReadDeadline(t: Date): void
        setWriteDeadline(t: Date): void
    }
}