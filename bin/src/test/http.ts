import { Command } from "../flags";
import *as http from "iotjs/net/http";
export const command = new Command({
    use: 'http',
    short: 'test http',
    prepare: (flags) => {
        const method = flags.string({
            name: "method",
            short: "m",
            default: "GET"
        })
        const body = flags.string({
            name: "body",
            short: "b",
            default: ""
        })
        const json = flags.bool({
            name: "json",
            short: "j",
        })
        return async (args: string[]) => {
            for (const url of args) {
                const at = Date.now()
                try {
                    const resp = await http.request(url, {
                        method: method.value as any,
                        body: body.value,
                        header: json ? {
                            "Accept": "application/json",
                            "Content-Type": "application/json",
                        } : undefined,
                    })
                    if (resp.body) {
                        console.log(new TextDecoder().decode(resp.body))
                    }
                    console.log(`code=${resp.code}`)
                    for (const key in resp.header) {
                        if (Object.prototype.hasOwnProperty.call(resp.header, key)) {
                            const val = resp.header[key];
                            console.log(`  ${key}: ${val}`)
                        }
                    }
                } catch (e) {
                    console.log(url, `${e}`)
                }
                const used = Date.now() - at
                console.log(`used ${used / 1000}s`)
            }
            http.close_idle()
        }
    }
})