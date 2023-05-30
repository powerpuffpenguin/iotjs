namespace iotjs {
    export * from "iotjs"
}

declare module "iotjs" {
    // iotjs 系統版本號
    export const version: string
    // 編譯目標 os
    export const os: string
    // 編譯目標 arch
    export const arch: 'amd64' | 'csky'
    // 啓動進程的命令參數
    export const argv: Array<string>
    // 大部分系統錯誤的基類
    export class IotError extends Error { }


    /**
     * 返回系統使用的 dns 服務器
     */
    export function nameserver(): string | undefined
    /**
     * 設置系統使用的 dns 服務器地址
     * @param addr 如果爲空白字符串，則使用 /etc/resolv.conf  的設定
     */
    export function nameserver(addr: string): void

    /**
     * 強制執行 gc，可能需要連續調用兩次
     * 
     * 第一輪將對象標記為可終結並運行終結器。 
     * 第二輪確保對像在完成後仍然無法訪問，然後釋放對象
     */
    export function gc(): void
    /**
     * 返回當前工作目錄
     */
    export function getcwd(): string
    /**
     * 調用 c 的 exit(code) 退出程式
     */
    export function exit(code: number): void

    export type BufferData = string | ArrayBuffer | DataView | Uint8Array | Uint8ClampedArray | Uint16Array | Uint32Array | Int8Array | Int16Array | Int32Array | Float32Array | Float64Array
}
declare module "iotjs/io" {
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
        n: number = 0
    }
    export class EofError extends IoError { }
    export interface Writer {
        write(p: Uint8Array): number | Promise<number>
    }
    export interface Reader {
        /**
         * 讀取數據到 p, 如果讀取到 eof 將 throw EofError
         * @param p 
         * @returns 返回隨機讀取到數據的長度
         */
        read(p: Uint8Array): number | Promise<number>
    }
    export interface Closer {
        close(): void | Promise<void>
    }
    export interface Seeker {
        seek(offset: number, whence: Seek): number | Promise<number>
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
        ReadFrom(r: Reader): number | Promise<number>
    }
    export interface WriterTo {
        WriteTo(w: Writer): number | Promise<number>
    }
    export interface ReaderAt {
        ReadAt(p: Uint8Array, off: number): number | Promise<number>
    }
    export interface WriterAt {
        WriteAt(p: Uint8Array, off: number): number | Promise<number>
    }
}
declare module "iotjs/encoding/hex" {
    export function encodeLen(n: number): number
    export function encodeToString(src: BufferData | string): string
    export function encode(dst: BufferData, src: BufferData | string): number
    export function decodedLen(x: number): number
    export function decodeString(s: string): Uint8Array;
    export function decode(dst: BufferData, src: BufferData): number
}
declare module "iotjs/hash" {
    import { BufferData } from "iotjs";
    import { Writer } from "iotjs/io";
    interface Hash extends Writer {
        /**
         * 返回當前 hash 寫入 b 後的最終值，但是它不會改變底層 hash 的狀態
         */
        sum(b?: BufferData | string): Uint8Array

        /**
         * 重置 hash
         */
        reset(): void

        /**
         * 返回 hash 長度
         */
        readonly size: number

        /**
         * 返回 hash 塊數據大小，write 可以接收任意長度的數據，但寫入 塊大小的整數倍可能會有更好的效率
         */
        readonly block: number
    }
}
declare module "iotjs/crypto/md4" {
    import { BufferData } from "iotjs";
    import { Hash } from "hash";
    /**
     * 返回 hash 長度
     */
    export const size: number
    /**
     * 返回 hash 塊數據大小，write 可以接收任意長度的數據，但寫入 塊大小的整數倍可能會有更好的效率
     */
    export const block: number
    /**
     * 計算 s 的 hash 並返回
     */
    export function sum(s: string | BufferData): Uint8Array
    /**
     * 創建一個 hash 對象用於計算大量數據的 hash 值
     */
    export function hash(params: type): Hash
}
declare module "iotjs/crypto/md5" {
    import { BufferData } from "iotjs";
    import { Hash } from "hash";
    /**
     * 返回 hash 長度
     */
    export const size: number
    /**
     * 返回 hash 塊數據大小，write 可以接收任意長度的數據，但寫入 塊大小的整數倍可能會有更好的效率
     */
    export const block: number
    /**
     * 計算 s 的 hash 並返回
     */
    export function sum(s: string | BufferData): Uint8Array
    /**
     * 創建一個 hash 對象用於計算大量數據的 hash 值
     */
    export function hash(params: type): Hash
}
declare module "iotjs/crypto/sha1" {
    import { BufferData } from "iotjs";
    import { Hash } from "hash";
    /**
     * 返回 hash 長度
     */
    export const size: number
    /**
     * 返回 hash 塊數據大小，write 可以接收任意長度的數據，但寫入 塊大小的整數倍可能會有更好的效率
     */
    export const block: number
    /**
     * 計算 s 的 hash 並返回
     */
    export function sum(s: string | BufferData): Uint8Array
    /**
     * 創建一個 hash 對象用於計算大量數據的 hash 值
     */
    export function hash(params: type): Hash
}
declare module "iotjs/crypto/sha224" {
    import { BufferData } from "iotjs";
    import { Hash } from "hash";
    /**
     * 返回 hash 長度
     */
    export const size: number
    /**
     * 返回 hash 塊數據大小，write 可以接收任意長度的數據，但寫入 塊大小的整數倍可能會有更好的效率
     */
    export const block: number
    /**
     * 計算 s 的 hash 並返回
     */
    export function sum(s: string | BufferData): Uint8Array
    /**
     * 創建一個 hash 對象用於計算大量數據的 hash 值
     */
    export function hash(params: type): Hash
}
declare module "iotjs/crypto/sha256" {
    import { BufferData } from "iotjs";
    import { Hash } from "hash";
    /**
     * 返回 hash 長度
     */
    export const size: number
    /**
     * 返回 hash 塊數據大小，write 可以接收任意長度的數據，但寫入 塊大小的整數倍可能會有更好的效率
     */
    export const block: number
    /**
     * 計算 s 的 hash 並返回
     */
    export function sum(s: string | BufferData): Uint8Array
    /**
     * 創建一個 hash 對象用於計算大量數據的 hash 值
     */
    export function hash(params: type): Hash
}
declare module "iotjs/crypto/sha384" {
    import { BufferData } from "iotjs";
    import { Hash } from "hash";
    /**
     * 返回 hash 長度
     */
    export const size: number
    /**
     * 返回 hash 塊數據大小，write 可以接收任意長度的數據，但寫入 塊大小的整數倍可能會有更好的效率
     */
    export const block: number
    /**
     * 計算 s 的 hash 並返回
     */
    export function sum(s: string | BufferData): Uint8Array
    /**
     * 創建一個 hash 對象用於計算大量數據的 hash 值
     */
    export function hash(params: type): Hash
}
declare module "iotjs/crypto/sha512" {
    import { BufferData } from "iotjs";
    import { Hash } from "hash";
    /**
     * 返回 hash 長度
     */
    export const size: number
    /**
     * 返回 hash 塊數據大小，write 可以接收任意長度的數據，但寫入 塊大小的整數倍可能會有更好的效率
     */
    export const block: number
    /**
     * 計算 s 的 hash 並返回
     */
    export function sum(s: string | BufferData): Uint8Array
    /**
     * 創建一個 hash 對象用於計算大量數據的 hash 值
     */
    export function hash(params: type): Hash
}
declare module "iotjs/crypto/sha512_224" {
    import { BufferData } from "iotjs";
    import { Hash } from "hash";
    /**
     * 返回 hash 長度
     */
    export const size: number
    /**
     * 返回 hash 塊數據大小，write 可以接收任意長度的數據，但寫入 塊大小的整數倍可能會有更好的效率
     */
    export const block: number
    /**
     * 計算 s 的 hash 並返回
     */
    export function sum(s: string | BufferData): Uint8Array
    /**
     * 創建一個 hash 對象用於計算大量數據的 hash 值
     */
    export function hash(params: type): Hash
}
declare module "iotjs/crypto/sha512_256" {
    import { BufferData } from "iotjs";
    import { Hash } from "hash";
    /**
     * 返回 hash 長度
     */
    export const size: number
    /**
     * 返回 hash 塊數據大小，write 可以接收任意長度的數據，但寫入 塊大小的整數倍可能會有更好的效率
     */
    export const block: number
    /**
     * 計算 s 的 hash 並返回
     */
    export function sum(s: string | BufferData): Uint8Array
    /**
     * 創建一個 hash 對象用於計算大量數據的 hash 值
     */
    export function hash(params: type): Hash
}
declare module "iotjs/crypto/tls" {
    /**
     * 用於配置 tls 客戶端或服務器，在被傳遞給 tls 函數後就不能再對它進行修改
     */
    export interface ConfigOptions {
        /**
         * 如果被設置爲 true 則客戶端不會驗證 tls 證書
         */
        insecureSkipVerify: boolean
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
        static open(name: string, flags = O_RDONLY, perm = 0): Promise<File>
        static openSync(name: string, flags = O_RDONLY, perm = 0): File
        static create(name: string, flags = O_RDWR | O_CREATE | O_TRUNC, perm = 0o666): Promise<File>
        static createSync(name: string, flags = O_RDWR | O_CREATE | O_TRUNC, perm = 0o666): File
    }
}
declare module "iotjs/net" {
    export const IPv4len = 4
    export const IPv6len = 16
    /**
     * 代表了一個 ip 地址
     */
    export class IP {
        /** 
         * 將 ip4 字符串轉爲 IP
         */
        static fromIP4(s: string): IP
        /** 
         * 將 ip6 字符串轉爲 IP
         */
        static fromIP6(s: string): IP
        /**
         * @param ip  bytes 長度必須爲 4 或 16
         */
        constructor(ip: Uint8Array)
        /**
         * 返回 存儲 ip 的字節
         */
        readonly ip: Uint8Array
        /**
         * 如果是 ipv4 返回 true
         */
        readonly ip4: boolean
        /**
         * 如果是 ipv6 返回 true
         */
        readonly ip6: boolean
        /**
         * 將 ip 轉爲人類友好的字符串
         */
        toString(): string
        /**
         * 類似 toString 但會添加一個版本前綴 ip4:[xxx] ip6:[xxx]
         */
        string(): string
    }
    export function resolveIP(network: 'ip' | 'ip4' | 'ip6', address: string): Promise<Array<IP>>
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
declare module "iotjs/net/http" {
    export interface RequestOptions {
        method?: 'GET' | 'HEAD' | 'POST' | 'PUT' | 'PATCH' | 'DELETE'
        header?: Record<string, string>
        body?: string | Uint8Array | ArrayBuffer
    }
    export class Response {
        code: number
        header?: Record<string, string>
        body?: Uint8Array
    }
    /**
     * 發送一個 請求
     * @param req 
     */
    export function request(url: string, opts: RequestOptions): Response | Promise<Response>

}