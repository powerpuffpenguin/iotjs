import { File } from "iotjs/fs";

async function main() {
    const f = await File.create("a.txt")
    console.log(f)
}
main()