var net = require("iotjs/net")
var WebsocketConn = net.WebsocketConn
var coroutine = require("iotjs/coroutine")

function sendJSON(co, dst, data) {
    return co.yield(function (notify) {
        var finish = false
        const cb = function (ok, e) {
            if (finish) {
                return
            }
            finish = true
            if (ok === undefined) {
                notify.error(e)
            } else {
                notify.value(ok)
            }
        };
        const ok = dst.send(JSON.stringify(data), cb)
        if (finish) {
            return
        }
        if (typeof ok === "boolean") {
            finish = true
            notify.value(ok)
        }
    })
}
function recv(co, src) {
    return co.yield(function (notify) {
        console.log('-------------recv')
        var finish = false
        const cb = function (data, e) {
            console.log("---------1", finish)
            if (finish) {
                return
            }
            finish = true
            if (data === undefined) {
                notify.error(e)
            } else {
                notify.value(data)
            }
        };
        console.log("do recv")
        const v = src.recv(cb)
        console.log("---------0", finish)
        if (finish) {
            return
        }
        if (typeof v === 'string' || v instanceof Uint8Array) {
            finish = true
            notify.value(v)
        }
    })

}

coroutine.go(function (co) {
    try {
        var access = 'CEUSFDUxMzM1Njc4OTAxMjM0NTY3OTY3GhQxMjM0NTY3ODkwMTIzNDU2Nzg3OCDktY-lBg.ni6uBjv_M6hrvK5rCK_vlA'
        var conn = co.yield(function (notify) {
            WebsocketConn.connect("wss://192.168.251.50:5050/api/v1/player/audio?player_token=Bearer%20" + access, {
                timeout: 1000,
                insecure: true,
            }, function (conn, e) {
                if (conn === undefined) {
                    notify.error(e)
                } else {
                    notify.value(conn)
                }
            })
        })
        console.log('connect success', new Date())
        sendJSON(co, conn, { what: 50 })
        console.log('send success', new Date())
        var data = recv(co, conn)
        console.log('recv success', new Date(), data)

        console.log("all success", new Date())
    } catch (e) {
        if (e instanceof Error) {
            console.log(e.toString())
        } else {
            console.log(e)
        }
    }
})
