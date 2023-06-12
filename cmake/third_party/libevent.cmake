message(STATUS "third party: libevent-2.1.12-stable")

set(items
    libevent.a
    libevent_pthreads.a
    libevent_openssl.a
)
foreach(item ${items})
    list(APPEND target_libs
        "${CMAKE_SOURCE_DIR}/${OUTPUT_ROOT_DIR}/libs/${item}"
    )
endforeach()