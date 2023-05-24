declare namespace deps {
    export class native extends Blob { }
    export const size: number
    export const block: number
    export function clone(n?: native): native
    export function reset(n: native): void
    export function write(n: native, s: Uint8Array): number
    export function done(n: native): Uint8Array
}
export const size = deps.size
export const block = deps.block
export function hash(): Hash {
    return new Hash(deps.clone())
}
export class Hash {
    readonly size = deps.size
    readonly block = deps.block
    constructor(private readonly n: deps.native) { }
    reset() {
        deps.reset(this.n)
    }
    sum(b?: Uint8Array): Uint8Array {
        const n = deps.clone(this.n)
        if (b && b.length) {
            deps.write(n, b)
        }
        return deps.done(n)
    }
    write(b: Uint8Array): number {
        if (!b.length) {
            return 0;
        }
        return deps.write(this.n, b)
    }
}