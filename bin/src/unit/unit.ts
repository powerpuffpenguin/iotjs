class Keys<K, V> {
    private keys_ = new Map<K, V>()
    private arrs_ = new Array<K>()
    get(k: K): undefined | V {
        return this.keys_.get(k)
    }
    put(k: K, v: V) {
        if (this.keys_.has(k)) {
            throw new Error(`key already exists: ${k}`)
        }
        this.keys_.set(k, v)
        this.arrs_.push(k)
    }
    keys() {
        return this.arrs_
    }
}
class AssertQuit { }
const assertQuit = new AssertQuit()
export class Assert {
    constructor(public readonly module: string, public readonly name: string) { }
    equal<T>(expect: T, actual: T, ...msg: Array<any>) {
        const s_expect = JSON.stringify(expect)
        const s_actual = JSON.stringify(actual)
        if (s_expect == s_actual) {
            return
        }
        console.log(`--- FAIL: ${this.name}`)
        console.log(`  Error: not equal`)
        console.log(`  Expect:`, s_expect)
        console.log(`  Actual:`, s_actual)
        if (msg.length != 0) {
            console.log(`  Message:`, ...msg)
        }
        const stack = new Error().stack
        if (typeof stack === "string") {
            console.log(`  Stack:`, stack)
        }
        throw assertQuit
    }
}

export class Module {
    private keys_ = new Keys<string, (assert: Assert) => void | Promise<void>>()
    constructor(public readonly name: string) { }
    /**
     * 註冊測試函數
     */
    test(name: string, run: (assert: Assert) => void | Promise<void>) {
        this.keys_.put(name, run)
    }
    async run(func: Array<string>, fail: boolean): Promise<[number, number]> {
        const at = Date.now()
        let passed = 0
        let failed = 0
        let matched = false
        let first = true
        for (const key of this.keys_.keys()) {
            if (func.length != 0) {
                matched = false
                for (const match of func) {
                    if (match.match(key)) {
                        matched = true
                        break
                    }
                }
                if (!matched) {
                    continue
                }
            }
            if (first) {
                first = false
                console.log(this.name)
            }
            const f = this.keys_.get(key)!
            const assert = new Assert(this.name, key)
            const at = Date.now()
            try {
                await f(assert)
                const used = (Date.now() - at) / 1000
                console.log(` - ${key} passed, used ${used}s`)
                passed++
            } catch (e) {
                const used = (Date.now() - at) / 1000
                if (e !== assertQuit) {
                    console.trace(`${e}`)
                }
                console.log(` - ${key} failed, used ${used}s`)
                failed++
            }
        }
        if (passed || failed) {
            const used = (Date.now() - at) / 1000
            console.log(` * ${passed} passed, ${failed} failed, used ${used}s`)
        }
        return [passed, failed]
    }
}
export class Test {
    private keys_ = new Keys<string, Module>()
    keys() {
        return this.keys_
    }
    /**
     * 返回測試模塊，如果不存在就創建一個新的
     */
    module(name: string): Module {
        let m = this.keys_.get(name)
        if (m) {
            return m
        }
        m = new Module(name)
        this.keys_.put(name, m)
        return m
    }
}

export const test = new Test();
/**
 * 運行所有註冊的測試代碼
 * @param module 設置要測試的模塊
 * @param func 設置要測試的函數
 * @param fail 如果爲 true 會忽略未通過的測試，繼續後續未完成的測試
 */
export async function run(module: Array<string>, func: Array<string>, fail: boolean) {
    const at = Date.now()
    let n = 0
    let passed = 0
    let failed = 0
    const keys = test.keys()
    let matched = false
    for (const key of keys.keys()) {
        if (module.length != 0) {
            matched = false
            for (const match of module) {
                if (match.match(key)) {
                    matched = true
                    break
                }
            }
            if (!matched) {
                continue
            }
        }
        const [v0, v1] = await keys.get(key)!.run(func, fail)
        if (!v0 && !v1) {
            continue
        }

        n++
        passed += v0
        failed += v1

        if (!fail && v1) {
            break
        }
    }
    const used = (Date.now() - at) / 1000
    console.log(`test ${n} modules, ${passed} passed, ${failed} failed, used ${used}s`)
    if (failed > 0) {
        iotjs.exit(1)
    }
}
