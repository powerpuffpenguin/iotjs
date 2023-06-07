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
/**
 * @alpha
 */
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
    export class NetError extends Error {
        eof?: boolean
    }
    export interface TCPConnOptions {
        /**
         * 如果爲 true 使用 tls
         */
        tls?: boolean
        /**
         * tls 時使用的 
         * sni hostname
         */
        hostname?: string
        /**
         * 在使用 tls 連接時不驗證證書合法性
         */
        insecure?: boolean
        /**
         * 連接超時毫秒數，小於 1 將不設置超時但通常系統 tcp 連接超時是 75s
         */
        timeout?: number
        /**
         *  當待讀取的 tcp 數據積壓到此值將停止接收數據
         */
        read?: number
        /**
         * 當待寫入的 tcp 數據積壓到此值，新的寫入將失敗
         */
        write?: number
    }
    /**
     * @alpha
     * 一個 tcp 連接
     * 
     * @remarks
     * 提供了三種方式用於數據讀取，通常你不應該混用它們除非你知道自己在做什麼
     * (混用可能導致奇怪的 bug，但正確的混用可以在執行效率和編程效率上取得最靈活的平衡)
     * 
     * 1. 設置 onMessage 回調，系統會自動讀取數據並回調此函數
     * 2. onMessage 每次都會創建一個新的 Uint8Array 用於讀取數據，你也可以設置 onWritable，它將在有數據可讀時被回調，
     *      應該在回調中使用 tryRead 讀取數據
     * 3. 調用 read 函數來讀取數據，如果沒有數據可讀它將返回一個 Promise 並在數據準備好後讀取數據並喚醒 Promise
     * 
     * 如果不設置 onMessage/onWritable 並且沒有調用 read 函數在，則可能無法獲取到 tcp 異常的通知
     */
    export class TCPConn {
        /**
         * 連接 socket 服務器
         * @param url 
         */
        static connect(hostname: string, port: number, opts?: TCPConnOptions): Promise<TCPConn>
        private constructor()

        /** 
         * 如果爲 true 則打印 調試數據
        */
        debug?: boolean
        /**
         * 設備關閉後自動回調，這個函數始終會被調用，你可以在此進行一些收尾的資源釋放工作
         * @remarks
         * 你不需要調用 this.close ，因爲連接資源已經被釋放之後才會調用此函數
         */
        onClose?: () => void
        /**
         * 連接出現錯誤時回調用於通知錯誤原因，如果是讀取到 eof 會傳入 undefined，否則傳入錯誤原因(通常是 NetError)
         * @remarks
         * 你不需要調用 this.close，在回調結束後系統會自動調用 this.close 釋放連接資源
         */
        onError?: (e?: any) => void
        /**
         * 當讀取到數據時回調
         */
        onMessage?: (data: Uint8Array) => void
        /**
         * 當寫入緩衝區已滿，客戶端將變得不可寫，並且 write 會失敗，當客戶端再次變得可寫時會回調此函數
         */
        onWritable?: () => void
        /**
         * 返回設備當前是否可寫
         */
        readonly readable: boolean
        /**
         * 當連接變得可讀時回調
         */
        onReadable?: () => void
        /**
         * 返回設備當前是否可讀
         */
        readonly readable: boolean
        /**
         * 爲底層設備設置 讀寫超時，只有在存在讀寫時才會調用此回調
         * @remarks
         * 例如當讀取緩衝區已滿，設備會自動停止接收網路數據這時不會調用讀取超時，因爲已經沒有讀取。
         * 類似如果寫入緩衝區爲空不存在寫入，這樣也不會調用寫入超時
         */
        onTimeout?: (read: boolean) => void
        /**
         * 手動關閉客戶端
         */
        close(): void
        /**
         * 返回設備是否已經關閉
         */
        readonly isClosed: boolean

        /**
         * 爲底層設置 讀寫緩衝區大小
         * @param read 如果爲 true 設置 讀取緩衝區否則設置 寫入緩衝區
         */
        setBuffer(read: boolean, n: number): void
        /**
         * 返回底層緩衝區大小
         */
        getBuffer(read: boolean): number
        /**
         * 設置讀寫超時毫秒，如果 <= 0 則禁用超時回調
         */
        setTimeout(read: number, write: number): void
        /**
         * 返回讀取/寫超時毫秒數，爲 0 表示禁用超時回調
         */
        getTimeout(): [number, number]

        /**
         * 發送一個數據幀，如果寫入緩衝區已滿則返回一個 Promise 用於異步寫入
         * @remarks
         * 這個函數和 read 類似，它比 tryWrite 開銷更大。當不可寫時會創建 Promise 並等待
         * 設備變得可寫後，進行寫入。它在內部調用底層的 tryWrite 如果失敗則監聽 底層的 onWritable 回調
         */
        write(data: string | Uint8Array | ArrayBuffer): number | Promise<number>
        /**
         * 嘗試寫入數據，返回實際寫入字節數，如果緩衝區已滿返回 0
         * @param s 
         */
        tryWrite(s: string | Uint8Array | ArrayBuffer): number
        /**
         * 嘗試讀取數據，返回實際讀取字節數，如果沒有數據返回 0
         * @param s 
         */
        tryRead(s: string | Uint8Array | ArrayBuffer): number
        /**
         * 讀取數據返回實際讀取的字節數，如果沒有數據返回一個 Promise 用於異步讀取
         * 
         * @remarks
         * 如果讀取到 eof 會返回 undefined，使用這個函數效率會比 onMessage 低很低，
         * 實際上它在內部使用了底層的 onMessage 回調，但是它每次都需要創建一個 Promise 這個開銷比單純的
         * onMessage 回調要大很低，所以如果邏輯簡單應該使用 onMessage。但是 read 比 onMessage 更容易處理複雜
         * 的邏輯，但這不是性能瓶頸時推薦使用 read 函數
         */
        read(data: Uint8Array | ArrayBuffer): number | Promise<number>
        /**
         * 當收到數據時回調
         */
        onMessage?: undefined | ((data: Uint8Array) => void)
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
        /**
         * 最大 5M，超過將 拋出異常
         */
        body?: Uint8Array
    }
    /**
     * 發送一個 http 請求
     * @param req 
     */
    export function request(url: string, opts: RequestOptions): Response | Promise<Response>

    /**
     * 關閉所有空閒的連接
     */
    export function close_idle(): void
    export interface WebsocketOptions {
        /**
         * 可設置此屬性覆蓋連接的 header Origin
         */
        origin?: string
        /**
         * 可設置此屬性覆蓋連接的 header Host
         */
        host?: string
        /**
         * 讀取到的單個消息最大長度
         * 默認爲 1024*1024
         */
        readlimit?: number
        /**
         * 連接超時毫秒數，小於 1 將不設置超時但通常系統 tcp 連接超時是 75s
         */
        timeout?: number
    }
    /**
     * @alpha
     * 一個 websocket 客戶端
     */
    export class Websocket {
        /**
         * 連接 websocket 服務器
         * @param url 
         */
        static connect(url: string, opts?: WebsocketOptions): Promise<Websocket>
        private constructor()
        /**
         * 數據接收回調，每當收到一個數據幀時調用此函數
         */
        onMessage: (data: string | Uint8Array) => void
        /**
         * 出現錯誤時回調此函數，如果是 eof 錯誤則 傳入 undefined
         */
        onClose: (e?: Error) => void
        /**
         * 當寫入緩衝區已滿，客戶端將變得不可寫，並且 write 會失敗，當客戶端再次變得可寫時會回調此函數
         */
        onWritable: () => void
        /**
         * 當讀寫超時時回調
         */
        onTimeout: (read: boolean) => void

        /**
         * 手動關閉客戶端
         */
        close(): void

        /**
         * 發送一幀數據
         * @param s 如果是字符串就發送文本數據否則發送二進制數據
         * @returns 成功返回 true，失敗返回 false 表示寫入緩衝區已滿，應該等到 onWritable 被回調後才能繼續寫入數據
         */
        send(s: string | Uint8Array | ArrayBuffer): boolean
        /**
         * 爲底層設置 讀寫緩衝區大小
         * @param read 
         * @param write 
         */
        setBuffer(read: number, write: number)
        /**
         * 返回底層讀寫緩衝區大小
         */
        getBuffer(): [/*read*/number,/*write*/ number]

        /**
         * 設置讀寫超時毫秒，如果小於 0 則禁用超時回調
         */
        setTimeout(read: number, write: number): void
        /**
         * 返回讀取設置的讀寫超時毫秒數，爲 0 表示禁用超時回調
         */
        getTimeout(): [/*read*/number,/*write*/ number]
        /**
         * 讀取一個數據幀，如果沒有數據幀則返回一個 Promise 用於異步讀取
         * 
         * @remarks
         * 如果讀取到 eof 會返回 undefined，使用這個函數效率會比 onMessage 低很低，
         * 實際上它在內部使用了底層的 onMessage 回調，但是它每次都需要創建一個 Promise 這個開銷比單純的
         * onMessage 回調要大很低，所以如果邏輯簡單應該使用 onMessage。但是 read 比 onMessage 更容易處理複雜
         * 的邏輯，但這不是性能瓶頸時推薦使用 read 函數
         */
        read(): undefined | Uint8Array | string | Promise<undefined | Uint8Array | string>
        /**
         * 發送一個數據幀，如果寫入緩衝區已滿則返回一個 Promise 用於異步寫入
         * @remarks
         * 這個函數和 read 類似，它比 send 開銷更大。當不可寫時會創建 Promise 並等待
         * 設備變得可寫後，進行寫入。它在內部調用底層的 send 如果失敗則監聽 底層的 onWritable 回調
         */
        write(s: string | Uint8Array | ArrayBuffer): number | Promise<number>
    }
}