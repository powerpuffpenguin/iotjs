namespace Duktape {
    export class Thread {
        __hash_duktape_thread: string
        constructor(yielder: (v?: any) => void);
        static resume<T>(t: Thread, v?: any): T
        static yield<T>(v?: any): T
    }
}
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
    export function exit(code: number): never

    /**
     * 手動觸發一個事件回調用於調用函數 f，next 將立刻返回，函數 f 會在捕獲到事件激活後進行調用，
     * 對於多次調用 next，傳入的多個函數 f 其調用順序是隨機的
     */
    export function next(f: () => void): void


    export type BufferData = string | ArrayBuffer | DataView | Uint8Array | Uint8ClampedArray | Uint16Array | Uint32Array | Int8Array | Int16Array | Int32Array | Float32Array | Float64Array
}
declare module "iotjs/bytes" {
    /**
     * 將 src 的數據以字節形式拷貝到 dst, 返回實際拷貝的字節數
     * @param dst 
     * @param src 
     */
    export function copy(dst: Uint8Array, src: Uint8Array | ArrayBuffer | string): number
    /**
     * 以字節形式比較 l r
     * @param l 
     * @param r 
     * @param icase 是否忽略大小寫
     * @returns l == r ? 0 ( l < r ? -1 : 1)
     */
    export function compare(l: Uint8Array | ArrayBuffer | string, r: Uint8Array | ArrayBuffer | string, icase = false): number

    /**
     * 將 dst 每個字節設置爲指定字符
     */
    export function fill(dst: Uint8Array, c: number): void
    /**
     * 如果 dst 中每個字符都是指定的字符 則返回 true 否則返回 false
     */
    export function is(dst: Uint8Array, c: number): boolean
}
/**
 * async await 源於 Promise，Promise 的完成回調必須在下一個 循環週期中調用，
 * 這通常不是太大的問題，但是在嵌入式模式下這種運行模式會太慢。
 * 
 * 好在 Duktape 提供了支持協程的工具，本喵對它進行了簡單的包裝，以便能使用比 async await 更高效且能用於嵌入式的協程。
 * 注意，一旦使用了協程代碼可能無法在其它js平臺上運行，因爲目前實現依賴與 Duktape 特性。
 * 
 * 這個協程庫的工作模式和 async await 不同它們的表現也不會一致，從運行角度來看，這個實現可能更像
 * golang 的協程(但並不支持多cpu)
 */
declare module "iotjs/coroutine" {
    /**
     * 協程上下文
     */
    export interface YieldContext {
        /**
         * 在調用函數 f 後，讓出 cpu 以便其它協程可以運行
         * @remarks
         * 通常這是在協程中調用一個異步函數，讓協程等待異步完成後再執行後續代碼，
         * 注意如果在協程之外調用行爲將是未定義的
         */
        yield<T>(f: (notify: ResumeContext<T>) => void): T
    }
    /**
     * 協程喚醒上下文，你應該調用一次 nextValue/nextError/next 來喚醒協程，
     * 多次調用將拋出異常
     */
    export interface ResumeContext<T> {
        /**
         * 喚醒協程並爲協程返回值 v
         */
        value(v: T): void
        /**
         * 喚醒協程並在協程中拋出異常
         */
        error(e?: any): void
        /**
         * 調用 resume 之後喚醒協程，並且 resume 函數的返回值作爲協程的返回值，
         * 如果 resume 函數拋出了任何異常，可以被協程捕獲
         */
        next(resume: () => T): void
    }
    /**
     * 啓動一個協程 執行函 f
     * @remarks
     * 如果創建協程失敗，可以使用 try catch 捕獲 go 拋出的異常
     * @param f 要執行的協程函數
     */
    export function go(f: (co: YieldContext) => void): Coroutine
    /**
     * 協程實現，通常直接調用 go 函數，除非你想監聽 協程 狀態
     */
    export class Coroutine implements YieldContext {
        constructor(f: (co: YieldContext) => void) { }
        /**
         * 協程當前狀態
         */
        readonly state: 'run' | 'yield' | 'finish'
        /**
         * 你可以設置這個回調函數，它將在協程函數返回後被調用
         */
        onFinish?: () => void
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
    export function hash(): Hash
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
    export function hash(): Hash
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
    export function hash(): Hash
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
    export function hash(): Hash
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
    export function hash(): Hash
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
    export function hash(): Hash
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
    export function hash(): Hash
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
    export function hash(): Hash
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
    export function hash(): Hash
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
    /**
     * 調用網路接口相關異常
     */
    export class NetError extends Error {
        /**
         * 如果爲 true 表示讀取到了 eof
         */
        eof?: boolean
        cancel?: boolean
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
         * 當待讀取的 tcp 數據積壓到此值將停止接收數據
         * 默認爲 1024*1024
         */
        read?: number
        /**
         * 當待寫入的 tcp 數據積壓到此值，新的寫入將失敗
         * 默認爲 1024*1024
         */
        write?: number
    }
    export class Cancel {
        /**
         * 取消操作
         */
        cancel(): void
    }
    /**
     * 一個 tcp 連接
     * 
     * @remarks
     * 提供了三種方式用於數據讀取，通常你不應該混用它們除非你知道自己在做什麼
     * (混用可能導致奇怪的 bug，但正確的混用可以在執行效率和編程效率上取得最靈活的平衡)
     * 
     * 1. 設置 onMessage 回調，系統會自動讀取數據並回調此函數
     * 2. onMessage 每次都會創建一個新的 Uint8Array 用於讀取數據，你也可以設置 onWritable，它將在有數據可讀時被回調，
     *      應該在回調中使用 tryRead 讀取數據
     * 3. 調用 read 函數來讀取數據，如果沒有數據可讀將等待數據可讀再自動讀取，並通過回調函數通知讀取結果
     * 
     */
    export class TCPConn {
        /**
         * 連接 socket 服務器
         * @param hostname 連接ip或域名
         * @param port 連接端口
         * @param opts 
         */
        static connect(hostname: string, port: number, opts: TCPConnOptions, cb: (conn?: TCPConn, e?: any) => void): void
        /**
         * 連接 socket 服務器
         * @param hostname 連接ip或域名
         * @param port 連接端口
         */
        static connect(hostname: string, port: number, cb: (conn?: TCPConn, e?: any) => void): void

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
         * 當寫入緩衝區已滿，客戶端將變得不可寫，並且 write 會失敗，當客戶端再次變得可寫時會回調此函數
         */
        onWritable?: () => void
        /**
         * 返回設備當前是否可寫
         */
        readonly writable: boolean
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
         * 發送數據
         * @remarks
         * 如果寫入成功返回寫入的字節數並且不會調用回調函數。
         * 如果緩衝區已滿會等待設備可寫後進行寫入，在寫入成功或發生錯誤後調用回調，
         * 此時會返回一個 Cancel，你可以在回調前調用 Cancel.cancel() 取消寫入
         */
        write(data: string | Uint8Array | ArrayBuffer, cb?: (n?: number, e?: any) => void): number | Cancel
        /**
         * 嘗試寫入數據，返回實際寫入字節數，如果緩衝區已滿返回 undefined
         * @param s 
         */
        tryWrite(s: string | Uint8Array | ArrayBuffer): number | undefined
        /**
         * 嘗試讀取數據，返回實際讀取字節數，如果沒有數據返回 undefined
         * @param s 
         */
        tryRead(s: Uint8Array | ArrayBuffer): number | undefined
        /**
         * 讀取數據
         * 
         * @remarks
         * 如果讀取到了數據返回讀取的字節數並且不會調用回調函數，
         * 如果沒有數據可讀會等待設備變得可讀後進行讀取，在讀取成功或發生錯誤後調用回調，
         * 此時會返回一個 Cancel，你可以在回調前調用 Cancel.cancel() 取消讀取
         */
        read(data: Uint8Array | ArrayBuffer, cb?: (n?: number, e?: any) => void): number | Cancel
        /**
         * 當收到數據時回調
         */
        onMessage?: undefined | ((data: Uint8Array) => void)
    }
    export interface WebsocketConnOptions {
        /**
         * 如果設置了此值，將直接連接此地址，而非從 url 解析 服務器地址
         */
        addr?: string
        /**
         * 如果設置了此值，將直接連接此端口，而非從 url 解析 服務器端口
         */
        port?: number
        /**
         * 可設置此屬性覆蓋連接的 header Origin
         */
        origin?: string
        /**
         * 可設置此屬性覆蓋連接的 header Host
         */
        host?: string
        /**
         * 在使用 tls 連接時不驗證證書合法性
         */
        insecure?: boolean
        /**
         * 連接超時毫秒數，小於 1 將不設置超時但通常系統 tcp 連接超時是 75s
         */
        timeout?: number
        /**
         * 當待讀取的 tcp 數據積壓到此值將停止接收數據
         * 默認爲 1024*1024
         */
        read?: number
        /**
         * 當待寫入的 tcp 數據積壓到此值，新的寫入將失敗
         * 默認爲 1024*1024
         */
        write?: number
        /**
         * 讀取到的單個消息最大長度
         * 默認爲 1024*1024
         */
        readlimit?: number
    }
    /**
     * 一個 websocket 連接
     */
    export class WebsocketConn {
        /**
         * 連接 websocket 服務器
         */
        static connect(url: string, opts: WebsocketConnOptions, cb: (conn?: WebsocketConn, e?: any) => void): void
        /**
         * 連接 websocket 服務器
         */
        static connect(url: string, cb: (conn?: WebsocketConn, e?: any) => void): void

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
         * 當寫入緩衝區已滿，客戶端將變得不可寫，並且 write 會失敗，當客戶端再次變得可寫時會回調此函數
         */
        onWritable?: () => void
        /**
         * 返回設備當前是否可寫
         */
        readonly writable: boolean
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
         * 寫入一幀數據
         * @remarks
         * 如果寫入成功返回 true 並且不會調用回調函數。
         * 如果緩衝區已滿會等待設備可寫後進行寫入，在寫入成功或發生錯誤後調用回調，
         * 此時會返回一個 Cancel，你可以在回調前調用 Cancel.cancel() 取消寫入
         */
        send(data: string | Uint8Array | ArrayBuffer, cb?: (ok?: boolean, e?: any) => void): boolean | Cancel
        /**
         * 嘗試寫入一幀數據，如果緩衝區已滿返回 false
         * @param s 
         */
        trySend(s: string | Uint8Array | ArrayBuffer): boolean
        /**
         * 嘗試讀取一個完整的 消息
         */
        tryRecv(): string | Uint8Array | undefined
        /**
         * 讀取一個完整的 消息 如果消息沒有收完
          * @remarks
         * 如果讀取到了數據返回讀取的數並且不會調用回調函數。
         * 如果沒有消息可讀會等待接收完整的消息，在讀取成功或發生錯誤後調用回調，
         * 此時會返回一個 Cancel，你可以在回調前調用 Cancel.cancel() 取消讀取
         */
        recv(cb?: (data?: string | Uint8Array, e?: any) => void): string | Uint8Array | Cancel
        /**
         * 當收到一個完整消息時回調
         */
        onMessage?: undefined | ((data: string | Uint8Array) => void)
    }
}
declare module "iotjs/net/http" {
    /**
     * 調用網路接口相關異常
     */
    export class HTTPError extends Error {
        /**
         * 調用 cancel 取消了請求
         */
        cancel?: boolean
    }
    export interface URL {
        scheme: string
        host: string
        port?: number
        userinfo?: string
        path: string
        query?: string
        fragment?: string
    }
    /**
     * 解析 url 字符串
     */
    export function parseURL(url: string): URL

    export interface ClientOptions {
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
    }
    /**
     * 一個 http 客戶端
     */
    export class Client {
        constructor(addr: string, port: number, readonly opts?: ClientOptions)
        /**
         * 返回連接是否關閉
         */
        readonly isClosed: boolean
        /**
         * 關閉連接
         */
        close(): void
        /**
         * 連接關閉後回調，你可以在此釋放相關資源
         */
        onClosed?: () => boolean
        /**
         * 發送一個 http 請求 
         */
        do(req: RequestOptions, cb?: (resp?: Response, e?: any) => void): Cancel
    }
    export interface RequestOptions {
        /**
         * 讀取到的單個消息最大長度
         * 默認爲 5*1024*1024
         */
        limit?: number
        /**
         * 請求路徑，默認 '/'
         */
        path?: string
        method?: 'GET' | 'HEAD' | 'POST' | 'PUT' | 'PATCH' | 'DELETE'
        header?: Record<string, string>
        body?: string | Uint8Array | ArrayBuffer
    }
    export interface Response {
        code: number
        header?: Record<string, string>
        body?: Uint8Array
    }
    export interface Cancel {
        cancel(e?: any): void
    }
}

/**
 * 提供了使用 mtd 訪問 flash 的能力
 */
declare module "iotjs/mtd" {
    export enum Seek {
        set = 0,
        cur = 1,
        end = 2,
    }
    export interface Info {
        type: number
        flags: number
        size: number
        erasesize: number
        writesize: number
        oobsize: number
    }
    /**
     * 用於打開一個 mtd 分區，以進行讀寫數據
     */
    export class File {
        /**
         * @param path 要打開的 mtd 分區路徑，例如 /dev/mtd0
         */
        constructor(readonly path: string)
        /**
         * 返回設備是否以及被關閉
         */
        readonly isClosed: boolean
        /**
         * 關閉設備
         */
        close(): void
        /**
         * 返回分區信息
         */
        info(): Info

        /**
         * 同步移動讀寫位置
         * @param offset 
         * @param whence 
         */
        seekSync(offset: number, whence: Seek): number
        /**
         * 使用 0xff 擦除數據 
         * @param offset 擦除起始位置, mtd 必須整塊擦除所以 offset % info().erasesize === 0
         * @param size 擦除大小, mtd 必須整塊擦除所以 offset % info().erasesize === 0
         */
        eraseSync(offset: number, size: number): void
        /**
         * 同步讀取數據,如果讀取到 eof 返回 0
         */
        readSync(data: Uint8Array): number
        /**
         * 同步寫入數據,返回實際寫入的字節數
         * @param data 
         */
        writeSync(data: Uint8Array | string | ArrayBuffer): number

        seek(offset: number, cb?: (ret?: number, e?: any) => void): void
        erase(offset: number, size: number, cb?: (e?: any) => void): void
        read(data: Uint8Array, cb?: (ret?: number, e?: any) => void): void
        write(data: Uint8Array | string | ArrayBuffer, cb?: (ret?: number, e?: any) => void): void
    }
}