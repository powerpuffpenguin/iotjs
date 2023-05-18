import { Command } from "../flags";
import { Used } from "./helper";
function fibonacci(n: number): number {
    if (n < 2) {
        return n
    }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

async function async_fibonacci(n: number): Promise<number> {
    if (n < 2) {
        return n
    }
    return await async_fibonacci(n - 1) + await async_fibonacci(n - 2)
}

export const command = new Command({
    use: 'fibonacci',
    short: 'test fibonacci',
    prepare(flags, cmd) {
        const fasync = flags.bool({
            name: 'async',
            short: 'a',
            usage: 'test async function',
        });
        const n = flags.number({
            name: 'number',
            short: 'n',
            usage: 'fibonacci(number)',
            default: 31,
        })
        return () => {
            const used = new Used()
            if (fasync.value) {
                async_fibonacci(n.value).then((exec) => {
                    used.commit()
                    console.log(`exec ${exec},used ${used}`)
                })
            } else {
                const exec = fibonacci(n.value)
                used.commit()
                console.log(`exec ${exec},used ${used}`)
            }
        }
    },
})