message(STATUS "third party: wolfssl-5.6.0-stable")

set(items
    libwolfssl.a
)
foreach(item ${items})
    list(APPEND target_libs
        "${CMAKE_SOURCE_DIR}/${OUTPUT_ROOT_DIR}/libs/${item}"
    )
endforeach()