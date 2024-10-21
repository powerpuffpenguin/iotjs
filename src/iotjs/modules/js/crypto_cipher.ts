declare namespace deps {
    const AES: number

    const CRYPT_OK: number
    const CRYPT_ERROR: number
    const CRYPT_NOP: number
    const CRYPT_INVALID_KEYSIZE: number
    const CRYPT_INVALID_ROUNDS: number
    const CRYPT_FAIL_TESTVECTOR: number
    const CRYPT_BUFFER_OVERFLOW: number
    const CRYPT_INVALID_PACKET: number
    const CRYPT_INVALID_PRNGSIZE: number
    const CRYPT_ERROR_READPRNG: number
    const CRYPT_INVALID_CIPHER: number
    const CRYPT_INVALID_HASH: number
    const CRYPT_INVALID_PRNG: number
    const CRYPT_MEM: number
    const CRYPT_PK_TYPE_MISMATCH: number
    const CRYPT_PK_NOT_PRIVATE: number
    const CRYPT_INVALID_ARG: number
    const CRYPT_FILE_NOTFOUND: number
    const CRYPT_PK_INVALID_TYPE: number
    const CRYPT_OVERFLOW: number
    const CRYPT_UNUSED1: number
    const CRYPT_INPUT_TOO_LONG: number
    const CRYPT_PK_INVALID_SIZE: number
    const CRYPT_INVALID_PRIME_SIZE: number
    const CRYPT_PK_INVALID_PADDING: number
    const CRYPT_HASH_OVERFLOW: number

    export class ECB {
        readonly __id = "ecb"
    }
    export function ecb(opts: ECBOptions): [ECB, number]
    export function ecb_encrypt(state: deps.ECB, dst: Uint8Array, src: Uint8Array | string): number
    export function ecb_decrypt(state: deps.ECB, dst: Uint8Array, src: Uint8Array | string): number


}
export interface Encryptor {
    encrypt(dst: Uint8Array, src: Uint8Array | string): void
}
export interface Decryptor {
    decrypt(dst: Uint8Array, src: Uint8Array | string): void
}
export const AES = deps.AES

function get(code: number): string {
    switch (code) {
        case deps.CRYPT_OK:
            return 'ok'
        case deps.CRYPT_ERROR:
            return 'generic error'
        case deps.CRYPT_NOP:
            return 'not a failure but no operation was performed'
        case deps.CRYPT_INVALID_KEYSIZE:
            return 'invalid key size given'
        case deps.CRYPT_INVALID_ROUNDS:
            return 'invalid number of rounds'
        case deps.CRYPT_FAIL_TESTVECTOR:
            return 'algorithm failed test vectors'
        case deps.CRYPT_BUFFER_OVERFLOW:
            return 'not enough space for output'
        case deps.CRYPT_INVALID_PACKET:
            return 'invalid input packet given'
        case deps.CRYPT_INVALID_PRNGSIZE:
            return 'invalid number of bits for a PRNG'
        case deps.CRYPT_ERROR_READPRNG:
            return 'could not read enough from PRNG'
        case deps.CRYPT_INVALID_CIPHER:
            return 'invalid cipher specified'
        case deps.CRYPT_INVALID_HASH:
            return 'invalid hash specified'
        case deps.CRYPT_INVALID_PRNG:
            return 'invalid PRNG specified'
        case deps.CRYPT_MEM:
            return 'out of memory'
        case deps.CRYPT_PK_TYPE_MISMATCH:
            return 'not equivalent types of PK keys'
        case deps.CRYPT_PK_NOT_PRIVATE:
            return 'requires a private PK key'
        case deps.CRYPT_INVALID_ARG:
            return 'generic invalid argument'
        case deps.CRYPT_FILE_NOTFOUND:
            return 'file not found'
        case deps.CRYPT_PK_INVALID_TYPE:
            return 'invalid type of PK key'
        case deps.CRYPT_OVERFLOW:
            return 'an overflow of a value was detected/prevented'
        case deps.CRYPT_UNUSED1:
            return 'unused1'
        case deps.CRYPT_INPUT_TOO_LONG:
            return 'the input was longer than expected'
        case deps.CRYPT_PK_INVALID_SIZE:
            return 'invalid size input for PK parameters'
        case deps.CRYPT_INVALID_PRIME_SIZE:
            return 'invalid size of prime requested'
        case deps.CRYPT_PK_INVALID_PADDING:
            return 'invalid padding on input'
        case deps.CRYPT_HASH_OVERFLOW:
            return 'hash applied to too many bits'
    }
    return `unknow ${code}`
}
export class CipherError extends Error {
    constructor(readonly code: number, options?: ErrorOptions | undefined) {
        super(get(code), options)
        // restore prototype chain   
        const proto = new.target.prototype
        if (Object.setPrototypeOf) {
            Object.setPrototypeOf(this, proto)
        }
        else {
            (this as any).__proto__ = proto
        }
        this.name = "CipherError"
    }
}

interface ECBOptions {
    /**
     * 算法索引，默認爲 AES
     */
    cipher?: number
    /**
     * 密鑰
     */
    key: Uint8Array | string
    /**
     * 加密輪數，不設置或爲 0 則使用默認輪數
     */
    rounds?: number
}
class ECBEncryptor {
    constructor(readonly state: deps.ECB) { }
    encrypt(dst: Uint8Array, src: Uint8Array | string): void {
        const e = deps.ecb_encrypt(this.state, dst, src)
        if (e != deps.CRYPT_OK) {
            throw new CipherError(e)
        }
    }
}
class ECBDecryptor {
    constructor(readonly state: deps.ECB) { }
    decrypt(dst: Uint8Array, src: Uint8Array | string): void {
        const e = deps.ecb_decrypt(this.state, dst, src)
        if (e != deps.CRYPT_OK) {
            throw new CipherError(e)
        }
    }
}
export function createECBEncryptor(opts: ECBOptions): Encryptor {
    const v = deps.ecb({
        cipher: opts.cipher ?? AES,
        key: opts.key,
        rounds: opts.rounds ?? 0,
    })
    if (v[1] != deps.CRYPT_OK) {
        throw new CipherError(v[1])
    }
    return new ECBEncryptor(v[0])
}
export function createECBDecryptor(opts: ECBOptions): Decryptor {
    const v = deps.ecb({
        cipher: opts.cipher ?? AES,
        key: opts.key,
        rounds: opts.rounds ?? 0,
    })
    if (v[1] != deps.CRYPT_OK) {
        throw new CipherError(v[1])
    }
    return new ECBDecryptor(v[0])
}