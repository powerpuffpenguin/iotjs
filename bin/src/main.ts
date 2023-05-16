import { Command, Parser } from "./flags";
import { command as timer } from "./test/timer";
import { command as fibonacci } from "./test/fibonacci";
const root = new Command({
    use: "main.js",
    short: "iotjs example and test",
    run(_, cmd) {
        cmd.print()
    },
});
root.add(
    timer,
    fibonacci,
)
new Parser(root).parse(process.argv.slice(2))