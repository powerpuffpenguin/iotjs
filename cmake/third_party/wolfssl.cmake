message(STATUS "third party: wolfssl-5.6.0-stable")

set(dir "${CMAKE_SOURCE_DIR}/${OUTPUT_ROOT_DIR}/wolfssl")
include_directories(${dir})
set (dir "${CMAKE_SOURCE_DIR}/${OUTPUT_ROOT_DIR}/wolfssl/src/.libs")
set(items
    libwolfssl.a
)
foreach(item ${items})
    list(APPEND target_libs
        "${dir}/${item}"
    )
endforeach()