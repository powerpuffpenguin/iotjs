export enum NetCode {
    NetworkInvalid
}
export class NetError extends Error {
    constructor(code: NetCode, message?: string | undefined, options?: ErrorOptions | undefined) {
        super(message, options)
    }
}
export async function resolveIP(network: 'ip' | 'ip4' | 'ip6', address: string): Promise<Uint8Array> {
    let ip: Uint8Array | undefined
    switch (network) {
        case 'ip4':
            ip = await _iotjs.dns.resolve_ip4(address)
            break
        case 'ip6':
            ip = await _iotjs.dns.resolve_ip6(address)
            break
        default:
            throw new NetError(NetCode.NetworkInvalid, "network invalid")
    }
    return ip;
}