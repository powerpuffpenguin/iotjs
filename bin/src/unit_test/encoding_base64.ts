import { test } from "../unit/unit";
import * as base64 from "iotjs/encoding/base64";
interface testpair {
    decoded: Uint8Array
    encoded: string
}
function makepair(decoded: string | Uint8Array, encoded: string) {
    return {
        decoded: typeof decoded === "string" ? new TextEncoder().encode(decoded) : decoded,
        encoded: encoded,
    }
}

const pairs: Array<testpair> = [
    // RFC 3548 examples
    makepair(new Uint8Array([0x14, 0xfb, 0x9c, 0x03, 0xd9, 0x7e]), "FPucA9l+"),
    makepair(new Uint8Array([0x14, 0xfb, 0x9c, 0x03, 0xd9]), "FPucA9k="),
    makepair(new Uint8Array([0x14, 0xfb, 0x9c, 0x03]), "FPucAw=="),

    // RFC 4648 examples
    makepair("", ""),
    makepair("f", "Zg=="),
    makepair("fo", "Zm8="),
    makepair("foo", "Zm9v"),
    makepair("foob", "Zm9vYg=="),
    makepair("fooba", "Zm9vYmE="),
    makepair("foobar", "Zm9vYmFy"),

    // Wikipedia examples
    makepair("sure.", "c3VyZS4="),
    makepair("sure", "c3VyZQ=="),
    makepair("sur", "c3Vy"),
    makepair("su", "c3U="),
    makepair("leasure.", "bGVhc3VyZS4="),
    makepair("easure.", "ZWFzdXJlLg=="),
    makepair("asure.", "YXN1cmUu"),
    makepair("sure.", "c3VyZS4="),
]

interface encodingTest {
    name: string
    enc: base64.Encoding           // Encoding to test
    conv: (s: string) => string // Reference string converter
}
function stdRef(s: string) {
    return s
}
function urlRef(s: string) {
    return s.replace(/\+/g, "-").replace(/\//g, "_")
}
function rawRef(s: string) {
    if (s == '') {
        return s
    }
    let i = s.length - 1
    for (; i >= 0; i--) {
        if (s[i] != '=') {
            break
        }
    }
    return s.substring(0, i + 1)
}
function rawURLRef(s: string) {
    return rawRef(urlRef(s))
}
const encodingTests: Array<encodingTest> = [
    { name: "std", enc: base64.std, conv: stdRef },
    { name: "url", enc: base64.url, conv: urlRef },
    { name: "raw std", enc: base64.rawSTD, conv: rawRef },
    { name: "raw url", enc: base64.rawURL, conv: rawURLRef },
]
const m = test.module("iotjs/encoding/base64")
m.test("TestEncode", (assert) => {
    for (const p of pairs) {
        for (const tt of encodingTests) {
            const got = tt.enc.encodeToString(p.decoded)
            assert.equal(tt.conv(p.encoded), got)
        }
    }
})
m.test("TestDecode", (assert) => {
    for (const p of pairs) {
        for (const tt of encodingTests) {
            const got = tt.enc.decode(tt.conv(p.encoded))
            assert.equal(p.decoded, got, tt.name, p.encoded)
        }
    }
})