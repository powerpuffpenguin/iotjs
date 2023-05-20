
import { Command } from "../flags";

export const command = new Command({
    use: 'test',
    short: 'unit test',
    prepare(flags, cmd) {
        const module = flags.string({
            name: 'module',
            short: 'm',
            usage: 'test module match',
        });
        const func = flags.string({
            name: 'func',
            short: 'f',
            usage: 'test func match',
        });
        return () => {

        }
    },
})