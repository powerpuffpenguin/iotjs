import { Command, Parser } from "./flags";
import { command as timer } from "./test/timer";
import { command as fibonacci } from "./test/fibonacci";
// import { command as fs } from "./test/fs";
// import { command as dns } from "./test/dns";
import { command as tcp } from "./test/tcp";
import { command as co } from "./test/co";
import { command as unit } from "./unit/command";
if (iotjs.arch === "csky") {
    iotjs.nameserver("192.168.251.1:53")
}
const root = new Command({
    use: "main.js",
    short: "iotjs example and test",
    run(_, cmd) {
        cmd.print()
    },
});
root.add(
    unit,
    timer,
    fibonacci,
    // fs,
    // dns,
    tcp,
    co,
)

new Parser(root).parse(iotjs.argv.slice(2))
