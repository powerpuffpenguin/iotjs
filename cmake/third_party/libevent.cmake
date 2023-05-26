message(STATUS "third party: libevent-2.1.12-stable")

set(dir "${CMAKE_SOURCE_DIR}/third_party/libevent-2.1.12-stable/include")
include_directories(${dir})
set (dir "${CMAKE_SOURCE_DIR}/${OUTPUT_ROOT_DIR}/libevent")
include_directories("${dir}/include")

set(items
    libevent.a
    libevent_pthreads.a
    libevent_openssl.a
)
foreach(item ${items})
    list(APPEND target_libs
        "${dir}/lib/${item}"
    )
endforeach()