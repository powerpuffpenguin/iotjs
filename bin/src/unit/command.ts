
import { Command } from "../flags";
import { run } from "./unit";
import "../unit_test/0";
export const command = new Command({
    use: 'test',
    short: 'unit test',
    prepare(flags, cmd) {
        const module = flags.strings({
            name: 'module',
            short: 'm',
            usage: 'test module match',
        });
        const func = flags.strings({
            name: 'func',
            short: 'f',
            usage: 'test func match',
        });
        const fail = flags.bool({
            name: 'fail',
            short: 'F',
            usage: 'Ignore the failure and continue with the unfinished test',
        })
        return () => {
            run(module.value, func.value, fail.value)
        }
    },
})