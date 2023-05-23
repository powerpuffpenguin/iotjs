import * as net from "iotjs/net";
import { Command } from "../flags";
class MyHelper {
    async v4(s: string) {
        try {
            const ips = await net.resolveIP('ip4', s)
            console.log(`resolve v4 ${s} success`)
            for (let i = 0; i < ips.length; i++) {
                const ip = ips[i]
                console.log(` ${i + 1}: ${ip}`)
            }
        } catch (e) {
            console.log(`resolve v4 ${s} error: ${e}`)
        }
    }
    async v6(s: string) {
        try {
            const ips = await net.resolveIP('ip6', s)
            console.log(`resolve v6 ${s} success`)
            for (let i = 0; i < ips.length; i++) {
                const ip = ips[i]
                console.log(` ${i + 1}: ${ip}`)
            }
        } catch (e) {
            console.log(`resolve v6 ${s} error: ${e}`)
        }
    }
    async ip(s: string) {
        try {
            const ips = await net.resolveIP('ip', s)
            console.log(`resolve ${s} success`)
            for (let i = 0; i < ips.length; i++) {
                const ip = ips[i]
                console.log(` ${i + 1}: ${ip}`)
            }
        } catch (e) {
            console.log(`resolve ${s} error: ${e}`)
        }
    }
}
export const command = new Command({
    use: 'dns',
    short: 'test iotjs/dns',
    prepare: (flags, cmd) => {
        const v4 = flags.strings({
            name: "v4",
            usage: "resolve ipv4",
        })
        const v6 = flags.strings({
            name: "v6",
            usage: "resolve ipv6",
        })
        return (args) => {
            const nameserver = iotjs.nameserver()
            if (nameserver) {
                console.log("nameserver:", nameserver)
            }
            const helper = new MyHelper()
            for (const v of v4.value) {
                helper.v4(v)
            }
            for (const v of v6.value) {
                helper.v6(v)
            }
            for (const ip of args) {
                helper.ip(ip)
            }
        }
    },
})