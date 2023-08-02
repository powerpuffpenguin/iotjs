var net = require("iotjs/net")
var WebsocketConn = net.WebsocketConn
var access = "123"
WebsocketConn.connect("wss://xsd-kb2022-dev.cdnewstar.cn:3443/api/v1/player/audio?player_token=Bearer%20" + access, {
    timeout: 1000,
    insecure: true,
}, function (conn, e) {
    if (e) {
        console.log(e)
    } else {
        console.log(conn.getPriority())
        conn.setPriority(0)
        console.log(conn.getPriority())
    }

})