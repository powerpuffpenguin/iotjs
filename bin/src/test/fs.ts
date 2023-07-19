// import * as fs from "iotjs/fs";
// import { Command } from "../flags";
// import { Helper } from "./helper";
// class MyHelper extends Helper {
//     private _FileMode() {
//         this.runSync("FileMode", () => {
//             const m = fs.FileMode
//             this.log(`${m.dir} === ${0x80000000}`, m.dir === 0x80000000)
//             this.log(`${m.irregular} === ${0x80000}`, m.irregular === 0x80000)
//             this.log(`${m.dir} === 0x80000000 `, m.dir === 0x80000000)
//             this.log(`${m.append} === 0x40000000 `, m.append === 0x40000000)
//             this.log(`${m.exclusive} === 0x20000000 `, m.exclusive === 0x20000000)
//             this.log(`${m.temporary} === 0x10000000 `, m.temporary === 0x10000000)
//             this.log(`${m.link} === 0x8000000 `, m.link === 0x8000000)
//             this.log(`${m.device} === 0x4000000 `, m.device === 0x4000000)
//             this.log(`${m.pipe} === 0x2000000 `, m.pipe === 0x2000000)
//             this.log(`${m.socket} === 0x1000000 `, m.socket === 0x1000000)
//             this.log(`${m.setuid} === 0x800000 `, m.setuid === 0x800000)
//             this.log(`${m.setgid} === 0x400000 `, m.setgid === 0x400000)
//             this.log(`${m.charDevice} === 0x200000 `, m.charDevice === 0x200000)
//             this.log(`${m.sticky} === 0x100000 `, m.sticky === 0x100000)
//             this.log(`${m.irregular} === 0x80000 `, m.irregular === 0x80000)
//             this.log(`${m.type} === 0x8f280000 `, m.type === 0x8f280000)
//             this.log(`${m.perm} === 0o777 `, m.perm === 0o777)
//         })
//     }
//     /**
//      * @override
//      */
//     async do() {
//         this._FileMode()
//         await this.run('stat', async () => {
//             const stat = await fs.stat(iotjs.argv[0])
//             this.log("stat:", stat)
//         })
//     }
//     /**
//      * @override
//      */
//     doSync() {
//         this._FileMode()
//         this.runSync('statSync', () => {
//             const stat = fs.statSync(iotjs.argv[0])
//             // const stat = fs.statSync("touch")
//             this.log("stat:", stat)
//         })
//     }
// }
// export const command = new Command({
//     use: 'fs',
//     short: 'test iotjs/fs',
//     prepare(flags, cmd) {
//         const fasync = flags.bool({
//             name: 'async',
//             short: 'a',
//             usage: 'test async function',
//         });
//         return () => {
//             const helper = new MyHelper()
//             if (fasync.value) {
//                 helper.exeute()
//             } else {
//                 helper.exeuteSync()
//             }
//         }
//     },
// })