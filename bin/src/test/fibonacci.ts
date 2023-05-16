import { Command } from "../flags";
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
            const at = Date.now()
            if (fasync.value) {
                async_fibonacci(n.value).then((exec) => {
                    const diff = Date.now() - at
                    console.log(`exec ${exec},used ${diff / 1000}s`)
                })
            } else {
                const exec = fibonacci(n.value)
                const diff = Date.now() - at
                console.log(`exec ${exec},used ${diff / 1000}s`)
            }
        }
    },
})