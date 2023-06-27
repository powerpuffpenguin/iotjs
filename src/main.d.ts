namespace Duktape {
    export class Thread {
        __hash_duktape_thread: string
        constructor(yielder: (v?: any) => void);
        static resume<T>(t: Thread, v?: any): T
        static yield<T>(v?: any): T
    }
}
declare namespace _iotjs {
    declare namespace path {
        export function join(...elem: Array<string>): string
        export function clean(path: string): string
        export function ext(path: string): string
        export function base(path: string): string
        export function split(path: string): Array<string>
        export function dir(path: string): string
        export function isAbs(path: string): boolean
    }
    /**
     * 讀取一個 utf8 檔案
     * @param path 
     */
    export function read_text(path): string
    /**
     * 檔案或檔案夾不存在 返回 undefined
     * @param path 
     * @returns 1 這是一個目錄，2 這是一個常規檔案，0 這個檔案不能作爲模塊
     */
    export function stat_module(path: string): 0 | 1 | 2 | undefined
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
    /**
     * 錯誤基類
     */
    export class IotError extends Error {
        constructor(message?: string | undefined, options?: ErrorOptions | undefined);
    }

    export interface OutputOptions {
        /**
         * prefix label
         */
        prefix: string
        /**
         * println current datetime
         */
        time: boolean
    }
    export interface Output {
        log(opts: OutputOptions, vals: Array<any>): void
    }
    const defaultOutput: Output
    export interface LoggerOptionsInit {
        /**
         * Log output target
         */
        output?: Output,

        /**
         * Whether to enable logging
         */
        enable?: boolean
        /**
         * prefix label
         */
        prefix?: string
        /**
         * println current datetime
         */
        time?: boolean
    }
    export interface LoggerOptions extends OutputOptions {
        /**
         * Log output target
         */
        output: Output
        /**
         * Whether to enable logging
         */
        enable: boolean
    }
    export class Logger {
        public readonly opts: LoggerOptions
        constructor(opts?: LoggerOptionsInit)
        log(...vals: Array<any>): void
    }
    export const defaultLogger: Logger
}