declare namespace deps {
    export class Encoding {
        readonly __iotjs_encoding_base64: string
    }
    export const encoder_std: string
    export const encoder_url: string
    export const decoder_std: Uint8Array
    export const decoder_url: Uint8Array

    export function encodedLen(len: number, padding: boolean): number
    export function decodedLen(len: number, padding: boolean): number

    export function encode(data: any, padding: undefined | string, encode: string): Uint8Array
    export function encodeToString(data: any, padding: undefined | string, encode: string): string

    export function decode(data: string | ArrayBuffer | Uint8Array, padding: undefined | string, decoder: Uint8Array, subarray: (data: Uint8Array, end: number) => Uint8Array): Uint8Array
    export function decodeToString(data: string | ArrayBuffer | Uint8Array, padding: undefined | string, decoder: Uint8Array, subarray: (data: Uint8Array, end: number) => Uint8Array): string
}
function subarray(data: Uint8Array, end: number) {
    return data.subarray(0, end)
}
export class Encoding {
    private padding: string
    private decoder: Uint8Array
    constructor(readonly encoder: string, padding?: any, decoder?: Uint8Array,) {
        if (padding !== undefined && padding !== null) {
            if (typeof padding !== "string") {
                padding = padding.toString()
            }
            this.padding = padding.length > 1 ? padding.substring(0, 1) : padding
        } else {
            this.padding = ""
        }
        if (encoder.length != 64) {
            throw new Error("invalid base64 encode")
        }
        if (decoder) {
            if (decoder.length != 256) {
                throw new Error("invalid base64 decoder")
            }
        } else {
            decoder = new Uint8Array(256)
            for (let i = 0; i < 256; i++) {
                decoder[i] = 0xff;
            }
            for (let i = 0; i < encoder.length; i++) {
                decoder[encoder.charCodeAt(i)] = i
            }
        }
        this.decoder = decoder
    }
    encodedLen(len: number): number {
        return deps.encodedLen(len, this.padding.length == 0 ? false : true)
    }
    encode(data: any): Uint8Array {
        return deps.encode(data, this.padding, this.encoder)
    }
    encodeToString(data: any): string {
        return deps.encodeToString(data, this.padding, this.encoder)
    }
    decodedLen(len: number): number {
        return deps.decodedLen(len, this.padding.length == 0 ? false : true)
    }
    decode(data: string | Uint8Array | ArrayBuffer): Uint8Array {
        return deps.decode(data, this.padding, this.decoder, subarray)
    }
    decodeToString(data: string | Uint8Array | ArrayBuffer): string {
        return deps.decodeToString(data, this.padding, this.decoder, subarray)
    }
}
export const std = new Encoding(deps.encoder_std, "=", deps.decoder_std)
export const rawSTD = new Encoding(deps.encoder_std, undefined, deps.decoder_std)
export const url = new Encoding(deps.encoder_url, "=", deps.decoder_url)
export const rawURL = new Encoding(deps.encoder_url, undefined, deps.decoder_url)