import { version } from "iotjs";
export class Animal {
    constructor(public name: string) { }
    speak() {
        console.log(`${this.name} speak ${version}`)
    }
}