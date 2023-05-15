import { Command, Parser } from "./flags";
import { command as timer } from "./test/timer";
const root = new Command({
    use: "main.js",
    short: "iotjs example and test",
    run(_, cmd) {
        cmd.print()
    },
});
root.add(
    timer,
)
new Parser(root).parse(process.argv.slice(1))