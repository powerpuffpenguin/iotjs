namespace iotjs {
    export * from "iotjs"
}

declare module "iotjs" {
    // iotjs 系統版本號
    export const version: string
    // 編譯目標 os
    export const os: string
    // 編譯目標 arch
    export const arch: string
    // 啓動進程的命令參數
    export const argv: Array<string>
    // 大部分系統錯誤的基類
    export class IotError extends Error { }
    /**
     * 返回當前工作目錄
     */
    export function getcwd(): string
    /**
     * 調用 c 的 exit(code) 退出程式
     */
    export function exit(code: number): void
    export class Int64 {
        constructor(o: string | number | Uint64 | Int64)
        toString(): string
        cmp(o: number | string | Int64): number
        add(o: number | string | Int64): Int64
        sub(o: number | string | Int64): Int64
        mul(o: number | string | Int64): Int64
        div(o: number | string | Int64): Int64
        not(): Int64
        or(o: number | string | Int64): Int64
        and(o: number | string | Int64): Int64
        lsh(o: number): Int64
        rsh(o: number, unsigned = false): Int64
    }
    export class Uint64 {
        constructor(o: string | number | Uint64 | Int64)
        toString(): string
        cmp(o: number | string | Uint64): number
        add(o: number | string | Uint64): Uint64
        sub(o: number | string | Uint64): Uint64
        mul(o: number | string | Uint64): Uint64
        div(o: number | string | Uint64): Uint64
        not(): Uint64
        or(o: number | string | Uint64): Uint64
        and(o: number | string | Uint64): Uint64
        lsh(o: number): Uint64
        rsh(o: number): Uint64
    }
    export type Int64Like = Int64 | number;
    export type Uint64Like = Uint64 | number;
}
declare module "iotjs/io" {
    import { Int64Like } from "iotjs";
    /**
     * Seek whence values.
     */
    export enum Seek {
        start = 0, // seek relative to the origin of the file
        current = 1, // seek relative to the current offset
        end = 2, // seek relative to the end
    }
    export class IoError extends Error {
        /**
         * 返回已經讀取或寫入的字節長度
         */
        n: Int64Like = 0
    }
    export class EofError extends IoError { }
    export interface Writer {
        write(p: Uint8Array): Int64Like | Promise<Int64Like>
    }
    export interface Reader {
        /**
         * 讀取數據到 p, 如果讀取到 eof 將 throw EofError
         * @param p 
         * @returns 返回隨機讀取到數據的長度
         */
        read(p: Uint8Array): Int64Like | Promise<Int64Like>
    }
    export interface Closer {
        close(): void | Promise<void>
    }
    export interface Seeker {
        seek(offset: Int64Like, whence: Seek): Int64Like | Promise<Int64Like>
    }
    export interface ReadWriter extends Reader, Writer { }
    export interface ReadCloser extends Reader, Closer { }
    export interface WriteCloser extends Writer, Closer { }
    export interface ReadWriteCloser extends Reader, Writer, Closer { }
    export interface ReadSeeker extends Reader, Seeker { }
    export interface ReadSeekCloser extends Reader, Seeker, Closer { }
    export interface WriteSeeker extends Writer, Seeker { }
    export interface ReadWriteSeeker extends Reader, Writer, Seeker { }
    export interface ReaderFrom {
        ReadFrom(r: Reader): Int64Like | Promise<Int64Like>
    }
    export interface WriterTo {
        WriteTo(w: Writer): Int64Like | Promise<Int64Like>
    }
    export interface ReaderAt {
        ReadAt(p: Uint8Array, off: Int64Like): Int64Like | Promise<Int64Like>
    }
    export interface WriterAt {
        WriteAt(p: Uint8Array, off: Int64Like): Int64Like | Promise<Int64Like>
    }
}
declare module "iotjs/encoding/hex" {
    export type Buffer = ArrayBuffer | DataView | Int8Array | Uint8Array | Uint8ClampedArray | Int16Array | Uint16Array | Int32Array | Uint32Array | Float32Array | Float64Array
    export function encodeLen(n: number): number
    export function encodeToString(src: Buffer): string
    export function encode(dst: Buffer, src: Buffer): number
    export function decodedLen(x: number): number
    export function decodeString(s: string): Uint8Array;
    export function decode(dst: Buffer, src: Buffer): number
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
    export const IPv4len = 4
    export const IPv6len = 16
    /**
     * 代表了一個 ip 地址
     */
    export class IP {
        // bytes 長度必須爲 4 或 16
        constructor(bytes: Uint8Array)
    }
    export function ipv4(a: number, b: number, c: number, d: number): IP
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