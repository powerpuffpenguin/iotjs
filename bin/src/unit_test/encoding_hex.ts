import { test } from "../unit/unit";
import * as hex from "iotjs/encoding/hex";

function make(enc: string, dec: Iterable<number>) {
    return {
        enc: enc,
        dec: new Uint8Array(dec)
    }
}
const encDecTests = [
    // make("", []),
    make("0001020304050607", [0, 1, 2, 3, 4, 5, 6, 7]),
    make("08090a0b0c0d0e0f", [8, 9, 10, 11, 12, 13, 14, 15]),
    make("f0f1f2f3f4f5f6f7", [0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7]),
    make("f8f9fafbfcfdfeff", [0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff]),
    make("67", ['g'.codePointAt(0)!]),
    make("e3a1", [0xe3, 0xa1]),
]

const m = test.module("iotjs/encoding/hex")
m.test('test_encode', (assert) => {
    for (let i = 0; i < encDecTests.length; i++) {
        const test = encDecTests[i]
        const dst = new Uint8Array(hex.encodeLen(test.dec.length))
        const n = hex.encode(dst, test.dec)
        assert.equal(n, dst.length)
        assert.equal(test.enc, new TextDecoder().decode(dst))
    }
})
m.test('test_decode', (assert) => {
    const decTests = new Array(...encDecTests)
    decTests.push(make("F8F9FAFBFCFDFEFF", [0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff]))
    for (let i = 0; i < decTests.length; i++) {
        const test = decTests[i]
        const dst = new Uint8Array(hex.decodedLen(test.enc.length))
        const n = hex.decode(dst, new TextEncoder().encode(test.enc))
        assert.equal(test.dec, dst)
    }
})
m.test('test_encodeToString', (assert) => {
    for (let i = 0; i < encDecTests.length; i++) {
        const test = encDecTests[i]
        const s = hex.encodeToString(test.dec)
        assert.equal(test.enc, s)
    }
})
m.test('test_decodeString', (assert) => {
    for (let i = 0; i < encDecTests.length; i++) {
        const test = encDecTests[i]
        const dst = hex.decodeString(test.enc)
        assert.equal(test.dec, dst)
    }
})