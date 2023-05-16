import { Command } from "../flags";
export const command = new Command({
    use: 'timer',
    short: 'test timer',
    run: () => {
        console.log("timer 1s")
        const timer = setTimeout(() => {
            console.error("never but execute timer 2s");
        }, 2000)
        let i = 0
        const interval = setInterval(() => {
            console.log(`interval ${i++}`)
        }, 100)
        setTimeout(() => {
            console.log("timer 1s ok")
            clearTimeout(timer)
            clearTimeout(timer)
            clearInterval(interval)
        }, 1000)
    }
})