#ifndef IOTJS_CORE_DEFINES_H
#define IOTJS_CORE_DEFINES_H

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef VM_IOTJS_OS
#define VM_IOTJS_OS "linux"
#endif
#ifndef VM_IOTJS_ARCH
#define VM_IOTJS_ARCH "amd64"
#endif

// 存儲導出給 js 的核心 c 代碼，用於實現 js 核心庫使用
#define VM_STASH_KEY_PRIVATE "_iotjs", 6
// 全局變量存儲了一些 iotjs 的 runtime 信息
#define VM_STASH_KEY_IOTJS "iotjs", 5
// 保存 native 實現的 js 模塊初始化函數
#define VM_STASH_KEY_NATIVE "native", 6
// 存儲 setTimeout 定時器
#define VM_STASH_KEY_TIMEOUT "timeout", 7
// 存儲 setInterval 定時器
#define VM_STASH_KEY_INTERVAL "interval", 8
// 存儲工作線程中正在執行的異步工作
#define VM_STASH_KEY_JOBS "jobs", 4
// 存儲的正在執行的異步請求
#define VM_STASH_KEY_ASYNC "async", 5
// 服務器 dns 設定
#define VM_STASH_KEY_NAMESERVER "nameserver", 10
#define VM_STASH_KEY_NEXT "next", 3

// 爲異步方法存儲 js 棧快照
#define VM_STASH_KEY_SNAPSHOTS "snapshots", 9
#define VM_SNAPSHOT_TCPCONN "TcpConn", 7
#define VM_SNAPSHOT_HTTP_CONN "HttpConn", 8
#define VM_SNAPSHOT_HTTP_REQUEST "HttpReq", 7
#define VM_SNAPSHOT_MTD "MTD", 3

#define VM_IOTJS_KEY_COMPLETER "Completer", 9
#define VM_IOTJS_KEY_ERROR "IotError", 8

// 檢測引用類型 有效
#define IOTJS_REFERENCE_VALID(var) (var.reference)
// 檢測引用類型指針 有效
#define IOTJS_REFERENCE_PTR_VALID(var) (var && var->reference)
// 檢測引用類型 無效
#define IOTJS_REFERENCE_INVALID(var) (!var.reference)
// 檢測引用類型指針 無效
#define IOTJS_REFERENCE_PTR_INVALID(var) (!var || !var->reference)

// 返回引用計數
#define IOTJS_REFERENCE_COUNT(var) (var.reference->count)

// 返回引用類型引用對象 的內存地址
#define IOTJS_REFERENCE_PTR(var) (var.reference->ptr + var.offset)
// 返回引用類型引用指針 的對象內存地址
#define IOTJS_REFERENCE_PTR_PTR(var) (var->reference->ptr + var->offset)

// 定義一個 struct 的實例，並將所有屬性設置爲 0
#define IOTJS_VAR_STRUCT(type, var) \
    type var;                       \
    memset(&var, 0, sizeof(type))

// 2**53-1
#define IOTJS_MAX_SAFE_INTEGER 9007199254740991
// -(2**53-1)
#define IOTJS_MIN_SAFE_INTEGER -9007199254740991

#define IOTJS_MAX_INT64 9223372036854775807
#define IOTJS_MIN_INT64 -9223372036854775808
#define IOTJS_MAX_UINT64 18446744073709551615
#define IOTJS_MAX_INT32 2147483647
#define IOTJS_MIN_INT32 -2147483648
#define IOTJS_MAX_UINT32 4294967295
#define IOTJS_MAX_INT16 32767
#define IOTJS_MIN_INT16 -32768
#define IOTJS_MAX_UINT16 65535
#define IOTJS_MAX_INT8 127
#define IOTJS_MIN_INT8 -128
#define IOTJS_MAX_UINT8 255

#define IOTJS_HEX_TABLE "0123456789abcdef"
#define IOTJS_HEX_REVERSS_TABLE(var) const char var[256] = {                                                                             \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}

#define IOTJS_FILEMODE_DIR 0x80000000
#define IOTJS_FILEMODE_APPEND 0x40000000
#define IOTJS_FILEMODE_EXCLUSIVE 0x20000000
#define IOTJS_FILEMODE_TEMPORARY 0x10000000
#define IOTJS_FILEMODE_LINK 0x8000000
#define IOTJS_FILEMODE_DEVICE 0x4000000
#define IOTJS_FILEMODE_PIPE 0x2000000
#define IOTJS_FILEMODE_SOCKET 0x1000000
#define IOTJS_FILEMODE_SETUID 0x800000
#define IOTJS_FILEMODE_SETGID 0x400000
#define IOTJS_FILEMODE_CHAR_DEVICE 0x200000
#define IOTJS_FILEMODE_STICKY 0x100000
#define IOTJS_FILEMODE_IR_REG 0x80000
#define IOTJS_FILEMODE_TYPE (IOTJS_FILEMODE_DIR | IOTJS_FILEMODE_LINK | IOTJS_FILEMODE_PIPE | IOTJS_FILEMODE_SOCKET | IOTJS_FILEMODE_DEVICE | IOTJS_FILEMODE_CHAR_DEVICE | IOTJS_FILEMODE_IR_REG)
#define IOTJS_FILEMODE_PERM 0777

#define IOTJS_SYSCALL_S_IFBLK 0x6000
#define IOTJS_SYSCALL_S_IFCHR 0x2000
#define IOTJS_SYSCALL_S_IFDIR 0x4000
#define IOTJS_SYSCALL_S_IFIFO 0x1000
#define IOTJS_SYSCALL_S_IFLNK 0xa000
#define IOTJS_SYSCALL_S_IFMT 0xf000
#define IOTJS_SYSCALL_S_IFREG 0x8000
#define IOTJS_SYSCALL_S_IFSOCK 0xc000

#define IOTJS_SYSCALL_S_ISGID 0x400
#define IOTJS_SYSCALL_S_ISUID 0x800
#define IOTJS_SYSCALL_S_ISVTX 0x200
#endif