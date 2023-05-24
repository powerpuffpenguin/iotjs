message(STATUS "third party: libtomcrypt-1.18.2")

set(dir "${CMAKE_SOURCE_DIR}/third_party/libtomcrypt-1.18.2")
include_directories("${dir}/src/headers")

list(APPEND target_libs
    "${CMAKE_SOURCE_DIR}/${OUTPUT_ROOT_DIR}/libtomcrypt/libtomcrypt.a"
)