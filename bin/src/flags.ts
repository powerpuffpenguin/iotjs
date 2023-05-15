
export class FlagsException extends Error {
    constructor(message: string) {
        super(message)
    }
}
const minpad = 8;
const matchUse = new RegExp(/^[a-zA-Z][a-zA-Z0-9\-_\.]*$/);
const matchFlagShort = new RegExp(/^[a-zA-Z0-9]$/);

function isShortFlag(v: string): boolean {
    if (v.length != 1) {
        return false;
    }
    const c = v.codePointAt(0);
    return c !== undefined && c < 128 && c > 0;
}
function compareString(l: string, r: string): number {
    if (l == r) {
        return 0;
    }
    return l < r ? -1 : 1;
}
/**
 * Command definition options
 */
export interface CommandOptions {
    /**
     * Command name
     * @remarks
     * The name must match the regular expression /^[a-zA-Z][a-zA-Z0-9\-_\.]*$/u, as you can see some special symbols are not allowed this is intentional, Because I think using strange special symbols as command names will give users a very bad experience
     * 
     */
    readonly use: string;
    /**
     * Short description of the command
     * @remarks
     * Line breaks are not allowed, because they will be displayed when listing subcommands. If there is a line break, it may disrupt the typesetting of the description information
     */
    readonly short?: string;

    /**
     * Detailed description of the command
     * 
     * @remarks
     * Here you can write detailed usage instructions including some usage examples
     */
    readonly long?: string;

    /**
     * Called when the user specifies to execute this command/subcommand
     */
    run?: (args: Array<string>, cmd: Command) => void;
    /**
     * If you want to define flags for a command, you should not specify the 'run' attribute but the 'prepare' attribute
     * @remarks
     * The passed 'flgas' is an example of the {@link Flags} that are associated with this command to specify flags for the command
     * 
     * If the callback function specified by prepare returns non-undefined, its return value will override the 'run' attribute, and if it returns undefined, it will not affect the run attribute
     */
    readonly prepare?: (
        flags: Flags,
        cmd: Command,
    ) => undefined | ((args: Array<string>, cmd: Command) => void);
}
export interface ParserOptions {
    /**
     * Whether to allow undefined flags
     * @defaultValue false
     */
    unknowFlags?: boolean;
    /**
     * Whether to allow undefined subcommands
     *  @defaultValue false
     */
    unknowCommand?: boolean;
}
/**
 * Represents a command or subcommand
 */
export class Command {
    private parent_?: Command;
    private children_?: Map<string, Command>;
    private flags_: Flags;
    /**
     * parameters not matched
     */
    readonly args = new Array<string>();
    constructor(public readonly opts: CommandOptions) {
        if (!matchUse.test(opts.use)) {
            throw new FlagsException(`use invalid: ${opts.use}`);
        }
        const short = opts.short;
        if (short !== undefined && short.indexOf("\n") != -1) {
            throw new FlagsException(`short invalid: ${short}`);
        }

        this.flags_ = Flags.make(this);
    }
    /**
     * Add subcommands to commands
     * @param cmds
     */
    add(...cmds: Array<Command>) {
        if (cmds.length == 0) {
            return;
        }
        let children = this.children_;
        if (!children) {
            children = new Map<string, Command>();
            this.children_ = children;
        }
        for (const cmd of cmds) {
            if (cmd.parent_) {
                throw new FlagsException(
                    `command "${cmd.opts.use}" already added to "${cmd.parent_!.flags().use
                    }"`,
                );
            }
            const opts = cmd.opts;
            if (opts.prepare) {
                const run = opts.prepare(cmd.flags(), cmd);
                if (run) {
                    opts.run = run;
                }
            }
            const key = opts.use;
            if (children.has(key)) {
                throw new FlagsException(`command "${key}" already exists`);
            } else {
                cmd.parent_ = this;
                cmd.flags();
                children.set(key, cmd);
            }
        }
    }
    /**
     * Returns the {@link Flags} associated with the command
     */
    flags(): Flags {
        return this.flags_;
    }
    /**
     * @internal
     */
    parse(args: Array<string>, opts?: ParserOptions) {
        if (opts === undefined) {
            opts = {};
        }
        this._parse(args, 0, args.length, opts);
    }
    private _parse(
        args: Array<string>,
        start: number,
        end: number,
        opts: ParserOptions,
    ) {
        // 重置解析狀態，以免多次解析緩存了上次的解析結果
        this.args.splice(0);
        const flags = this.flags();
        flags.reset();

        if (end - start < 1) {
            const run = this.opts.run;
            if (run) {
                run(this.args, this);
            }
            return;
        }
        // 解析子命令和參數
        const children = this.children_;
        for (let i = start; i < end; i++) {
            const arg = args[i];
            if (arg == "-" || arg == "--") {
                if (opts.unknowFlags) {
                    continue;
                }
                throw new FlagsException(
                    `unknown flag in ${flags.use}: ${arg}`,
                );
            }
            if (arg.startsWith("-")) { // 解析參數
                // 解析內置特定參數
                if (arg == "-h") {
                    this._print();
                    return;
                }
                // 解析用戶定義參數
                const val = i + 1 < end ? args[i + 1] : undefined;
                if (arg.startsWith("--")) {
                    // 解析長參數
                    if (arg == "--help") {
                        const h = this._parseHelp(flags, "--help", val);
                        if (h == -1) {
                            this._print();
                            return;
                        }
                        i += h;
                        continue;
                    }

                    i += this._parseLong(flags, arg.substring(2), val, opts);
                } else {
                    // 解析短參數
                    if (arg == "-h") {
                        const h = this._parseHelp(flags, "-h", val);
                        if (h == -1) {
                            this._print();
                            return;
                        }
                        i += h;
                        continue;
                    }

                    const h = this._parseShort(flags, arg.substring(1), val, opts);
                    if (h == -1) { //help
                        this._print();
                        return;
                    }
                    i += h;
                }
            } else if (children) { // 解析子命令
                const sub = children.get(arg);
                if (sub) {
                    sub._parse(args, i + 1, end, opts);
                    return;
                } else {
                    if (opts.unknowCommand) {
                        return;
                    }
                    throw new FlagsException(`unknow commnad <` + arg + `>`);
                }
            } else { // 沒有子命令才允許傳入 args
                this.args.push(arg);
            }
        }
        const run = this.opts.run;
        if (run) {
            run(this.args, this);
        }
    }

    private _throw(
        flags: Flags,
        flag: FlagDefine<any>,
        arg: string,
        val?: string,
    ): never {
        if (val === undefined && !flag.isBool()) {
            throw new FlagsException(
                `flag in ${flags.use} needs an argument: ${arg}`,
            );
        }
        if (val === undefined) {
            val = "";
        } else {
            val = ` ${val}`;
        }
        throw new FlagsException(
            `invalid flag value in ${flags.use}: ${arg}${val}`,
        );
    }
    private _parseHelp(flags: Flags, arg: string, val?: string): number {
        if (val === undefined || val === "true") {
            return -1;
        } else if (val === "false") {
            return 1;
        }
        if (val === undefined) {
            throw new FlagsException(
                `flag in ${flags.use} needs an argument: ${arg}`,
            );
        }
        if (val === undefined) {
            val = "";
        } else {
            val = ` ${val}`;
        }
        throw new FlagsException(
            `invalid flag value in ${flags.use}: ${arg}${val}`,
        );
    }
    private _parseShortOne(
        flags: Flags,
        arg: string,
        val: string | undefined,
        opts: ParserOptions,
    ): number {
        if (arg == "h") {
            return this._parseHelp(flags, `-${arg}`, val);
        }
        const flag = flags.find(arg, true);
        if (!flag) {
            if (opts.unknowFlags) {
                return 1;
            }
            throw new FlagsException(
                `unknown flag in ${flags.use}: -${arg}`,
            );
        }
        if (flag.isBool()) {
            if (val !== "false" && val !== "true") {
                val = undefined;
            }
        }
        if (flag.add(val)) {
            return val === undefined ? 0 : 1;
        }
        this._throw(flags, flag, `-${arg}`, val);
    }
    private _parseShort2(
        flags: Flags,
        arg: string,
        val: string,
        opts: ParserOptions,
    ): number {
        if (arg == "h") {
            const v = this._parseHelp(flags, "-h", val);
            return v == -1 ? v : 0;
        }
        const flag = flags.find(arg, true);
        if (!flag) {
            if (opts.unknowFlags) {
                return 0;
            }
            throw new FlagsException(
                `unknown flag in ${flags.use}: -${arg}`,
            );
        }
        if (flag.add(val)) {
            return 0;
        }
        this._throw(flags, flag, `-${arg}`, val);
    }
    private _parseShort(
        flags: Flags,
        arg: string,
        nextVal: string | undefined,
        opts: ParserOptions,
    ): number {
        switch (arg.length) {
            case 0:
                if (opts.unknowFlags) {
                    return 0;
                }
                throw new FlagsException(
                    `unknown flag in ${flags.use}: -${arg}`,
                );
            case 1:
                return this._parseShortOne(flags, arg, nextVal, opts);
        }
        if (arg[1] == "=") {
            return this._parseShort2(flags, arg[0], arg.substring(2), opts);
        }
        const name = arg[0];
        const flag = flags.find(name, true);
        if (!flag) {
            if (opts.unknowFlags) {
                return 0;
            }
            throw new FlagsException(
                `unknown flag in ${flags.use}: -${name}`,
            );
        } else if (!flag.isBool()) {
            return this._parseShort2(flags, arg[0], arg.substring(1), opts);
        }
        if (flag.add(undefined)) {
            return this._parseShort(flags, arg.substring(1), nextVal, opts);
        }
        throw new FlagsException(
            `invalid flag value in ${flags.use}: ${name}`,
        );
    }
    private _parseLong(
        flags: Flags,
        arg: string,
        val: string | undefined,
        opts: ParserOptions,
    ): number {
        const found = arg.indexOf("=");
        let name: string;
        let next = false;
        if (found == -1) {
            name = arg;
            next = true;
        } else {
            name = arg.substring(0, found);
            val = arg.substring(found + 1);
        }

        const flag = flags.find(name);
        if (!flag) {
            if (opts.unknowFlags) {
                return next ? 1 : 0;
            }
            throw new FlagsException(
                `unknown flag in ${flags.use}: --${name}`,
            );
        }
        if (next && flag.isBool()) {
            if (val !== "false" && val !== "true") {
                next = false;
                val = undefined;
            }
        }
        if (flag.add(val)) {
            return next ? 1 : 0;
        }
        this._throw(flags, flag, `--${name}`, val);
    }
    private _print() {
        console.log(this.toString());
    }
    /**
     * Get the description string of command usage
     */
    toString(): string {
        const opts = this.opts;
        const use = this.flags().use;
        const strs = new Array<string>();
        const long = opts.long ?? "";
        const short = opts.short ?? "";
        if (long == "") {
            if (short != "") {
                strs.push(short);
            }
        } else {
            strs.push(long);
        }
        if (strs.length == 0) {
            strs.push("Usage:");
        } else {
            strs.push("\nUsage:");
        }
        strs.push(`  ${use} [flags]`);
        const children = this.children_;
        if (children) {
            strs.push(`  ${use} [command]

Available Commands:`);
            const arrs = new Array<Command>();
            let pad = 0;
            for (const v of children.values()) {
                const len = v.opts.use.length ?? 0;
                if (len > pad) {
                    pad = len;
                }
                arrs.push(v);
            }
            pad += 3;
            if (pad < minpad) {
                pad = minpad;
            }
            arrs.sort((l, r) => compareString(l.opts.use, r.opts.use));
            for (const child of arrs) {
                const opts = child.opts;
                strs.push(`  ${opts.use.padEnd(pad)}${opts.short}`);
            }
        }

        const flags = this.flags();
        let sp = 1;
        let lp = 4;
        for (const f of flags) {
            if (sp < f.short.length) {
                sp = f.short.length;
            }
            if (lp < f.name.length) {
                lp = f.name.length;
            }
        }
        if (lp < minpad) {
            lp = minpad;
        }
        strs.push(`\nFlags:
  -${"h".padEnd(sp)}, --${"help".padEnd(lp)}   help for ${opts.use}`);
        for (const f of flags) {
            let s = "";
            let str = f.defaultString();
            if (str != "") {
                s += " " + str;
            }
            str = f.valuesString();
            if (str != "") {
                s += " " + str;
            }

            if (f.short == "") {
                strs.push(
                    `   ${"".padEnd(sp)}  --${f.name.toString().padEnd(lp)
                    }   ${f.usage}${s}`,
                );
            } else {
                strs.push(
                    `  -${f.short.toString().padEnd(sp)}, --${f.name.toString().padEnd(lp)
                    }   ${f.usage}${s}`,
                );
            }
        }
        if (children) {
            strs.push(
                `\nUse "${use} [command] --help" for more information about a command.`,
            );
        }
        return strs.join("\n");
    }
    /**
     * Use console.log output usage
     */
    print() {
        console.log(this.toString());
    }
    /**
     * return parent command
     */
    parent(): Command | undefined {
        return this.parent_;
    }
}
/**
 * Flags definition and parse
 */
export class Flags implements Iterable<FlagDefine<any>> {
    /**
    * @internal
    */
    static make(cmd: Command): Flags {
        return new Flags(cmd)
    }
    private constructor(
        private cmd: Command,
    ) { }
    /**
     * @internal
     */
    get use(): string {
        const cmd = this.cmd;
        let parent = cmd.parent();
        let use = cmd.opts.use;
        while (parent) {
            use = `${parent.opts.use} ${use}`;
            parent = parent.parent();
        }
        return use;
    }
    private short_?: Map<string, FlagDefine<any>>;
    private long_?: Map<string, FlagDefine<any>>;
    private arrs_?: Array<FlagDefine<any>>;
    /**
     * @internal
     */
    find(name: string, short = false): FlagDefine<any> | undefined {
        return short ? this.short_?.get(name) : this.long_?.get(name);
    }
    private _getArrs(): undefined | Array<FlagDefine<any>> {
        const keys = this.long_;
        if (!keys) {
            return;
        }
        let arrs = this.arrs_;
        if (!arrs || arrs.length != keys.size) {
            arrs = [];
            for (const f of keys.values()) {
                arrs.push(f);
            }
            arrs.sort((l, r) => compareString(l.name, r.name));
        }
        return arrs;
    }
    /**
     * @internal
     */
    iterator(): Iterator<FlagDefine<any>, undefined> {
        const arrs = this._getArrs();
        let i = 0;
        return {
            next() {
                if (arrs && i < arrs.length) {
                    return { value: arrs[i++] };
                }
                return { done: true };
            },
        };
    }
    /**
     * @internal
     */
    [Symbol.iterator](): Iterator<FlagDefine<any>> {
        return this.iterator();
    }
    /**
     * @internal
     */
    reset() {
        this.long_?.forEach((f) => {
            f.reset();
        });
    }
    /**
     * Define flags
     */
    add(...flags: Array<FlagDefine<any>>) {
        if (flags.length == 0) {
            return;
        }
        let kl = this.long_;
        if (!kl) {
            kl = new Map<string, FlagDefine<any>>();
            this.long_ = kl;
        }
        let ks = this.short_;
        if (!ks) {
            ks = new Map<string, FlagDefine<any>>();
            this.short_ = ks;
        }
        for (const f of flags) {
            const name = f.name;

            if (kl.has(name)) {
                throw new FlagsException(`${this.use} flag redefined: ${name}`);
            }
            const short = f.short;
            if (short !== "") {
                const found = ks.get(short);
                if (found) {
                    throw new FlagsException(
                        `unable to redefine '${short}' shorthand in "${this.use}" flagset: it's already used for "${found.name}" flag`,
                    );
                }
                if (!isShortFlag(short)) {
                    throw new FlagsException(
                        `"${short}" shorthand in "${this.use} is more than one ASCII character`,
                    );
                }
                ks.set(short, f);
            }
            kl.set(name, f);
        }
    }
    /**
     * Define a flag of type string
     */
    string(opts: FlagOptionsLike<string>): FlagString {
        const f = new FlagString(opts);
        this.add(f);
        return f;
    }
    /**
     * Define a flag of type Array<string>
     */
    strings(opts: FlagOptionsLike<Array<string>>): FlagStrings {
        const f = new FlagStrings(opts);
        this.add(f);
        return f;
    }
    /**
     * Define a flag of type number
     */
    number(opts: FlagOptionsLike<number>): FlagNumber {
        const f = new FlagNumber(opts);
        this.add(f);
        return f;
    }
    /**
     * Define a flag of type Array<number>
     */
    numbers(opts: FlagOptionsLike<Array<number>>): FlagNumbers {
        const f = new FlagNumbers(opts);
        this.add(f);
        return f;
    }
    /**
     * Define a flag of type bigint
     */
    bigint(opts: FlagOptionsLike<bigint>): FlagBigint {
        const f = new FlagBigint(opts);
        this.add(f);
        return f;
    }
    /**
     * Define a flag of type Array<bigint>
     */
    bigints(opts: FlagOptionsLike<Array<bigint>>): FlagBigints {
        const f = new FlagBigints(opts);
        this.add(f);
        return f;
    }
    /**
     * Define a flag of type boolean
     */
    bool(opts: FlagOptionsLike<boolean>): FlagBoolean {
        const f = new FlagBoolean(opts);
        this.add(f);
        return f;
    }
    /**
     * Define a flag of type Array<boolean>
     */
    bools(opts: FlagOptionsLike<Array<boolean>>): FlagBooleans {
        const f = new FlagBooleans(opts);
        this.add(f);
        return f;
    }
}
export interface FlagOptionsLike<T> {
    /**
     * flag long name
     * @remarks
     * The name must match the regular expression /^[a-zA-Z][a-zA-Z0-9\-_\.]*$/u, as you can see some special symbols are not allowed this is intentional, Because I think using strange special symbols as flags names will give users a very bad experience
     */
    readonly name: string;
    /**
     * Default value when no flag is specified
     */
    readonly default?: T;
    /**
     * Optional flag short name
     * @remarks
     * The short name must match the regular expression /^[a-zA-Z0-9]$/u, as you can see symbols are not allowed this is intentional, Because I think using symbols as flags short names will give users a very bad experience
     */
    readonly short?: string;
    /**
     * Optional flag usage description
     */
    readonly usage?: string;
    /**
     * An optional list of valid values for the flag
     * @remarks
     * If isValid is set at the same time, isValid will be called for verification when the values do not match, and if the values match, the value will be considered valid and will not call isValid
     */
    readonly values?: Array<T>;
    /**
     * Optional parameter validation function
     * @remarks
     * If values is set at the same time, isValid will be called for verification when the values do not match, and if the values match, the value will be considered valid and will not call isValid
     */
    readonly isValid?: (v: T) => boolean;
}
export interface FlagOptions<T> {
    /**
     * flag long name
     */
    readonly name: string;
    /**
     * Default value when no flag is specified
     */
    readonly default: T;
    /**
     * Optional flag short name
     */
    readonly short?: string;
    /**
     * Optional flag usage description
     */
    readonly usage?: string;
    /**
     * An optional list of valid values for the flag
     * @remarks
     * If isValid is set at the same time, isValid will be called for verification when the values do not match, and if the values match, the value will be considered valid and will not call isValid
     */
    readonly values?: Array<T>;
    /**
     * Optional parameter validation function
     * @remarks
     * If values is set at the same time, isValid will be called for verification when the values do not match, and if the values match, the value will be considered valid and will not call isValid
     */
    readonly isValid?: (v: T) => boolean;
}
export interface FlagDefine<T> {
    readonly name: string;
    readonly default: T;
    readonly short: string;
    readonly usage: string;
    readonly values?: Array<T>;
    isValid: (v: T) => boolean;
    /**
     * set value to default value
     */
    reset(): void;
    /**
     * The value parsed by this flag
     */
    readonly value: T;
    /**
     * Returns a string description of the default value
     */
    defaultString(): string;
    /**
     * Returns a string description of the list of valid values
     */
    valuesString(): string;
    /**
     * Parsed to this flag
     * @param val The parsed command line value
     */
    add(val?: string): boolean;
    /**
     * Returns whether it is a bool flag
     */
    isBool(): boolean;
}
/**
 * A base class provides some common methods for the class flag
 */
export class FlagBase<T> implements FlagDefine<T> {
    protected value_: T;
    get value(): T {
        return this.value_;
    }
    constructor(
        public readonly opts: FlagOptions<T>,
    ) {
        if (
            opts.short !== undefined &&
            opts.short !== "" &&
            !matchFlagShort.test(opts.short)
        ) {
            throw new FlagsException(
                `"${opts.short}" shorthand should match "^[a-zA-Z0-9]$"`,
            );
        }
        if (!matchUse.test(opts.name)) {
            throw new FlagsException(
                `"${opts.name}" flag should match "^[a-zA-Z][a-zA-Z0-9\\-_\\.]*$"`,
            );
        }
        if (opts.usage !== undefined && opts.usage.indexOf("\n") != -1) {
            throw new FlagsException(`flag usage invalid: ${opts.usage}`);
        }

        if (Array.isArray(opts.default)) {
            const a = Array.from(opts.default);
            this.value_ = a as any;
        } else {
            this.value_ = opts.default;
        }
    }
    get short(): string {
        return this.opts.short ?? "";
    }
    get name(): string {
        return this.opts.name;
    }
    get default(): T {
        return this.opts.default;
    }
    get usage(): string {
        return this.opts.usage ?? "";
    }
    get values(): Array<T> | undefined {
        return this.opts.values;
    }
    isValid(v: T): boolean {
        if (typeof v === "number") {
            if (!isFinite(v)) {
                return false;
            }
        }
        if (Array.isArray(v)) {
            for (const i of v) {
                if (!isFinite(i)) {
                    return false;
                }
            }
        }

        const opts = this.opts;
        const values = opts.values;
        if (values && values.length != 0) {
            for (const val of values) {
                if (this._equal(v, val)) {
                    return true;
                }
            }
            const f = opts.isValid;
            if (f) {
                return f(v);
            }
            return false;
        }
        const f = opts.isValid;
        if (f) {
            return f(v);
        }
        return true;
    }
    private _equal(l: T, r: T): boolean {
        if (Array.isArray(l) && Array.isArray(r)) {
            if (l.length != r.length) {
                return false;
            }
            for (let i = 0; i < l.length; i++) {
                if (l[i] !== r[i]) {
                    return false;
                }
            }
        }
        return l === r;
    }
    reset(): void {
        const def = this.opts.default;
        if (Array.isArray(def)) {
            const arrs = this.value_;
            if (Array.isArray(arrs)) {
                arrs.splice(0);
                arrs.push(...def);
                return;
            }
        }
        this.value_ = this.opts.default;
    }
    defaultString(): string {
        const val = this.opts.default;
        if (Array.isArray(val)) {
            if (val.length != 0) {
                return `(default ${JSON.stringify(val)})`;
            }
        } else if (typeof val === "string") {
            if (val != "") {
                return (`(default ${JSON.stringify(val)})`);
            }
        } else if (typeof val === "boolean") {
            if (val) {
                return (`(default ${val})`);
            }
        } else if (typeof val === "number") {
            if (val != 0) {
                return (`(default ${val})`);
            }
        } else if (typeof val === "bigint") {
            if (val != BigInt(0)) {
                return (`(default ${val})`);
            }
        }
        return "";
    }
    valuesString(): string {
        const vals = this.opts.values;
        if (vals && vals.length != 0) {
            return `(values ${JSON.stringify(vals)})`;
        }
        return "";
    }
    add(_?: string): boolean {
        return false;
    }
    isBool(): boolean {
        return false;
    }
}

function formatFlagOptions<T>(
    opts: FlagOptionsLike<T>,
    def: T,
): FlagOptions<T> {
    if (opts.default !== undefined) {
        return opts as any;
    }
    return {
        name: opts.name,
        default: def,
        short: opts.short,
        usage: opts.usage,
        values: opts.values,
        isValid: opts.isValid,
    };
}
/**
 * A flag of type string
 */
export class FlagString extends FlagBase<string> {
    constructor(opts: FlagOptionsLike<string>) {
        super(formatFlagOptions(opts, ""));
    }
    /**
     * @override
     */
    add(v?: string): boolean {
        if (v === undefined || !this.isValid(v)) {
            return false;
        }
        this.value_ = v;
        return true;
    }
}
/**
 * A flag of type Array<string>
 */
export class FlagStrings extends FlagBase<Array<string>> {
    constructor(opts: FlagOptionsLike<Array<string>>) {
        super(formatFlagOptions(opts, []));
    }
    /**
     * @override
     */
    add(v?: string): boolean {
        if (v === undefined || !this.isValid([v])) {
            return false;
        }
        this.value_.push(v);
        return true;
    }
}
/**
 * A flag of type number
 */
export class FlagNumber extends FlagBase<number> {
    constructor(opts: FlagOptionsLike<number>) {
        super(formatFlagOptions(opts, 0));
    }
    /**
     * @override
     */
    add(v?: string): boolean {
        if (v === undefined) {
            return false;
        }
        const i = parseInt(v);
        if (!this.isValid(i)) {
            return false;
        }
        this.value_ = i;
        return true;
    }
}
/**
 * A flag of type Array<number>
 */
export class FlagNumbers extends FlagBase<Array<number>> {
    constructor(opts: FlagOptionsLike<Array<number>>) {
        super(formatFlagOptions(opts, []));
    }
    /**
     * @override
     */
    add(v?: string): boolean {
        if (v === undefined) {
            return false;
        }
        const i = parseInt(v);
        if (!this.isValid([i])) {
            return false;
        }
        this.value_.push(i);
        return true;
    }
}
/**
 * A flag of type bigint
 */
export class FlagBigint extends FlagBase<bigint> {
    constructor(opts: FlagOptionsLike<bigint>) {
        super(formatFlagOptions(opts, BigInt(0)));
    }
    /**
     * @override
     */
    add(v?: string): boolean {
        if (v === undefined) {
            return false;
        }
        try {
            const i = BigInt(v);
            if (!this.isValid(i)) {
                return false;
            }
            this.value_ = i;
            return true;
        } catch (_) {
            return false;
        }
    }
}
/**
 * A flag of type Array<bigint>
 */
export class FlagBigints extends FlagBase<Array<bigint>> {
    constructor(opts: FlagOptionsLike<Array<bigint>>) {
        super(formatFlagOptions(opts, []));
    }
    /**
     * @override
     */
    add(v?: string): boolean {
        if (v === undefined) {
            return false;
        }
        try {
            const i = BigInt(v);
            if (!this.isValid([i])) {
                return false;
            }
            this.value_.push(i);
            return true;
        } catch (_) {
            return false;
        }
    }
}
function parseBool(v?: string): boolean | undefined {
    if (v === undefined) {
        return true;
    } else if (v === "true") {
        return true;
    } else if (v === "false") {
        return false;
    }
    return undefined;
}
/**
 * A flag of type boolean
 */
export class FlagBoolean extends FlagBase<boolean> {
    constructor(opts: FlagOptionsLike<boolean>) {
        super(formatFlagOptions(opts, false));
    }
    /**
     * @override
     */
    isBool(): boolean {
        return true;
    }
    /**
     * @override
     */
    add(v?: string): boolean {
        const val = parseBool(v);
        if (val === undefined || !this.isValid(val)) {
            return false;
        }
        this.value_ = val;
        return true;
    }
}
/**
 * A flag of type Array<boolean>
 */
export class FlagBooleans extends FlagBase<Array<boolean>> {
    constructor(opts: FlagOptionsLike<Array<boolean>>) {
        super(formatFlagOptions(opts, []));
    }
    /**
     * @override
     */
    isBool(): boolean {
        return true;
    }
    /**
     * @override
     */
    add(v?: string): boolean {
        const val = parseBool(v);
        if (val === undefined || !this.isValid([val])) {
            return false;
        }
        this.value_.push(val);
        return true;
    }
}

/**
 * command parser
 */
export class Parser {
    constructor(public readonly root: Command) {
        const opts = root.opts;
        const prepare = opts.prepare;
        if (prepare) {
            const run = prepare(root.flags(), root);
            if (run) {
                opts.run = run;
            }
        }
    }
    /**
     * Parses command line arguments and invokes a handler callback for a matching command or its subcommands
     * @param args command line parameters
     * @param opts some optional behavior definitions
     * 
     * @throws {@link FlagsException}
     * 
     * @example deno
     * ```
     * new Parser(root).parse(Deno.args)
     * ```
     * 
     * @example nodejs
     * ```
     * new Parser(root).parse(process.argv.splice(2))
     * ```
     */
    parse(args: Array<string>, opts?: ParserOptions) {
        this.root.parse(args, opts);
    }
}