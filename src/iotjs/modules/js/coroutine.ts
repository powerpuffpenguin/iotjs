export function safeRun<T>(f: () => T): T {
    try {
        return f()
    } catch (e) {
        if (e instanceof Error) {
            console.error(e.toString())
        } else {
            console.error(e)
        }
        _iotjs.exit(1)
    }
}

/**
 * 協程上下文
 */
export interface YieldContext {
    /**
     * 在調用函數 f 後，讓出 cpu 以便其它協程可以運行
     * @remarks
     * 通常這是在協程中調用一個異步函數，讓協程等待異步完成後再執行後續代碼
     */
    yield<T>(f: (notify: ResumeContext<T>) => void): T
}
/**
 * 協程喚醒上下文，你應該調用一次 nextValue/next 來喚醒協程，
 * 多次調用將拋出異常
 */
export interface ResumeContext<T> {
    /**
     * 喚醒協程並爲協程返回值 v
     */
    nextValue(v: T): void
    /**
     * 調用 resume 之後喚醒協程，並且 resume 函數的返回值作爲協程的返回值，
     * 如果 resume 函數拋出了任何異常，可以被協程捕獲
     */
    next(resume: () => T): void
}
class resumeContext<T> {
    constructor(private next_: (v: yieldValue<T>) => void) {
    }
    private resume_ = 0
    nextValue(v: T): void {
        if (this.resume_) {
            throw new Error("Coroutine already resume");
        }
        this.resume_ = 1
        this.next_({
            v: v,
        })
    }
    next(resume: () => T): void {
        if (this.resume_) {
            throw new Error("Coroutine already resume");
        }
        this.resume_ = 1
        try {
            const v = resume()
            this.next_({
                v: v,
            })
        } catch (e) {
            this.next_({
                e: e,
                err: true,
            })
        }
    }
}
interface yieldValue<T> {
    v?: any
    e?: any
    err?: boolean
}
export class Coroutine implements YieldContext {
    private t_: Duktape.Thread
    private y_: (v: any) => any
    onFinish?: () => void
    constructor(f: (co: YieldContext) => void) {
        const t = new Duktape.Thread(() => {
            const y = Duktape.Thread.yield
            y(y)
            f(this)

            const on = this.onFinish
            if (on) {
                safeRun(() => on())
            }
        })
        this.t_ = t
        this.y_ = Duktape.Thread.resume(t)
        _iotjs.next(() => safeRun(() => this._next()))
    }
    private state_: 'run' | 'yield' | 'finish' = 'run'
    get state(): string {
        return this.state_
    }
    yield<T>(f: (notify: ResumeContext<T>) => void): T {
        if (this.state_ != 'run') {
            throw new Error(`yield on ${this.state_}`)
        } else if (typeof f != 'function') {
            throw new TypeError('yield expects a function')
        }
        this.state_ = 'yield'
        const v: yieldValue<T> = this.y_(f)
        if (v.err) {
            throw v.e;
        }
        return v.v
    }
    private _next(v?: any) {
        const f: undefined | ((notify: ResumeContext<any>) => void) = safeRun(() => Duktape.Thread.resume(this.t_, v))
        if (!f) {
            this.state_ = 'finish'
            return
        }
        try {
            f(new resumeContext((v: any) => {
                if (this.state_ != 'yield') {
                    this._next({
                        err: true,
                        e: new Error(`resume on ${this.state_}`),
                    })
                    return
                }
                this.state_ = 'run'
                this._next(v)
            }))
        } catch (e) {
            this.state_ = 'run'
            this._next({
                err: true,
                e: e,
            })
        }
    }
}
/**
 * 啓動一個協程
 */
export function go(f: (co: YieldContext) => void): Coroutine {
    return new Coroutine(f)
}