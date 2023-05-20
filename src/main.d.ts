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
    export function exit(code: number): void
    /**
     * 錯誤基類
     */
    export class IotError extends Error {
        constructor(message?: string | undefined, options?: ErrorOptions | undefined);
    }
    declare namespace dns {
        export function resolve_ip4(address: string): Promise<Uint8Array>;
        export function resolve_ip6(address: string): Promise<Uint8Array>;
    }
}