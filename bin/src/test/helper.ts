export class Used {
    begin = Date.now()
    end?: number
    diff = 0
    commit() {
        if (this.end !== undefined) {
            this.begin = this.end
        }
        const now = Date.now()
        this.end = now
        this.diff = (now - this.begin) / 1000
    }
    toString() {
        return `${this.diff}s`
    }
}
export class Helper {
    async exeute() {
        const used = new Used()
        await this.do()
        console.log(`used ${used}`)
    }
    exeuteSync() {
        const used = new Used()
        this.doSync()
        console.log(`used ${used}`)
    }
    /**
     * @virtual
     */
    async do() { }
    /**
     * @virtual
     */
    doSync() { }

    async run(name: string, f: () => Promise<void>) {
        console.log(`---    ${name} ---`)
        const used = new Used()
        await f()
        used.commit()
        console.log(` * used ${used}\n`)
    }
    async runSync(name: string, f: () => void) {
        console.log(`---    ${name} ---`)
        const used = new Used()
        f()
        used.commit()
        console.log(` * used ${used}\n`)
    }
    log(...vals: Array<any>) {
        console.log(' -', ...vals)
    }

}
