import { Command, Parser } from "./flags";
import { command as timer } from "./test/timer";
import { command as fibonacci } from "./test/fibonacci";
import { command as fs } from "./test/fs";
import { command as dns } from "./test/dns";
import { command as unit } from "./unit/command";
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
    fs,
    dns,
)

new Parser(root).parse(iotjs.argv.slice(2))