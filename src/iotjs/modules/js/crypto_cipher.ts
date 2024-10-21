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

    const CRYPT_INVALID_IVSIZE: number

    export class ECB {
        readonly __id = "ecb"
    }
    export function ecb(opts: ECBOptions): [ECB, number]
    export function ecb_memory(state: deps.ECB, dst: Uint8Array, src: Uint8Array | string, enc: boolean): number

    export class CBC {
        readonly __id = "cbc"
    }
    export function cbc(opts: CBCOptions): [CBC, number]
    export function cbc_memory(state: deps.CBC, dst: Uint8Array, src: Uint8Array | string, enc: boolean): number

    export class CFB {
        readonly __id = "cfb"
    }
    export function cfb(opts: CFBOptions): [CFB, number]
    export function cfb_memory(state: deps.CFB, dst: Uint8Array, src: Uint8Array | string, enc: boolean): number

    export class OFB {
        readonly __id = "ofb"
    }
    export function ofb(opts: OFBOptions): [OFB, number]
    export function ofb_memory(state: deps.OFB, dst: Uint8Array, src: Uint8Array | string, enc: boolean): number
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

        case deps.CRYPT_INVALID_IVSIZE:
            return 'invalid iv size given'
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

export interface ECBOptions {
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

function createECB(opts: ECBOptions): deps.ECB {
    const v = deps.ecb({
        cipher: opts.cipher ?? AES,
        key: opts.key,
        rounds: opts.rounds ?? 0,
    })
    if (v[1] != deps.CRYPT_OK) {
        throw new CipherError(v[1])
    }
    return v[0]
}
export class ECBEncryptor {
    private readonly state_: deps.ECB
    constructor(opts: ECBOptions) {
        this.state_ = createECB(opts)
    }
    encrypt(dst: Uint8Array, src: Uint8Array | string): void {
        const e = deps.ecb_memory(this.state_, dst, src, true)
        if (e != deps.CRYPT_OK) {
            throw new CipherError(e)
        }
    }
}
export class ECBDecryptor {
    private readonly state_: deps.ECB
    constructor(opts: ECBOptions) {
        this.state_ = createECB(opts)
    }
    decrypt(dst: Uint8Array, src: Uint8Array | string): void {
        const e = deps.ecb_memory(this.state_, dst, src, false)
        if (e != deps.CRYPT_OK) {
            throw new CipherError(e)
        }
    }
}

export interface CBCOptions extends ECBOptions {
    /**
     * 初始化向量
     */
    iv: Uint8Array | string
}
function createCBC(opts: CBCOptions): deps.CBC {
    const v = deps.cbc({
        cipher: opts.cipher ?? AES,
        key: opts.key,
        rounds: opts.rounds ?? 0,
        iv: opts?.iv,
    })
    if (v[1] != deps.CRYPT_OK) {
        throw new CipherError(v[1])
    }
    return v[0]
}
export class CBCEncryptor {
    private readonly state_: deps.CBC
    constructor(opts: CBCOptions) {
        this.state_ = createCBC(opts)
    }
    encrypt(dst: Uint8Array, src: Uint8Array | string): void {
        const e = deps.cbc_memory(this.state_, dst, src, true)
        if (e != deps.CRYPT_OK) {
            throw new CipherError(e)
        }
    }
}
export class CBCDecryptor {
    private readonly state_: deps.CBC
    constructor(opts: CBCOptions) {
        this.state_ = createCBC(opts)
    }
    decrypt(dst: Uint8Array, src: Uint8Array | string): void {
        const e = deps.cbc_memory(this.state_, dst, src, false)
        if (e != deps.CRYPT_OK) {
            throw new CipherError(e)
        }
    }
}
export type CFBOptions = CBCOptions
function createCFB(opts: CBCOptions): deps.CFB {
    const v = deps.cfb({
        cipher: opts.cipher ?? AES,
        key: opts.key,
        rounds: opts.rounds ?? 0,
        iv: opts?.iv,
    })
    if (v[1] != deps.CRYPT_OK) {
        throw new CipherError(v[1])
    }
    return v[0]
}
export class CFBEncryptor {
    private readonly state_: deps.CFB
    constructor(opts: CFBOptions) {
        this.state_ = createCFB(opts)
    }
    encrypt(dst: Uint8Array, src: Uint8Array | string): void {
        const e = deps.cfb_memory(this.state_, dst, src, true)
        if (e != deps.CRYPT_OK) {
            throw new CipherError(e)
        }
    }
}
export class CFBDecryptor {
    private readonly state_: deps.CFB
    constructor(opts: CFBOptions) {
        this.state_ = createCFB(opts)
    }
    decrypt(dst: Uint8Array, src: Uint8Array | string): void {
        const e = deps.cfb_memory(this.state_, dst, src, false)
        if (e != deps.CRYPT_OK) {
            throw new CipherError(e)
        }
    }
}
export type OFBOptions = CBCOptions
function createOFB(opts: OFBOptions): deps.OFB {
    const v = deps.ofb({
        cipher: opts.cipher ?? AES,
        key: opts.key,
        rounds: opts.rounds ?? 0,
        iv: opts?.iv,
    })
    if (v[1] != deps.CRYPT_OK) {
        throw new CipherError(v[1])
    }
    return v[0]
}
export class OFBEncryptor {
    private readonly state_: deps.OFB
    constructor(opts: OFBOptions) {
        this.state_ = createOFB(opts)
    }
    encrypt(dst: Uint8Array, src: Uint8Array | string): void {
        const e = deps.ofb_memory(this.state_, dst, src, true)
        if (e != deps.CRYPT_OK) {
            throw new CipherError(e)
        }
    }
}
export class OFBDecryptor {
    private readonly state_: deps.OFB
    constructor(opts: OFBOptions) {
        this.state_ = createOFB(opts)
    }
    decrypt(dst: Uint8Array, src: Uint8Array | string): void {
        const e = deps.ofb_memory(this.state_, dst, src, false)
        if (e != deps.CRYPT_OK) {
            throw new CipherError(e)
        }
    }
}