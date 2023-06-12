message(STATUS "third party: libtomcrypt-1.18.2")

list(APPEND target_libs
    "${CMAKE_SOURCE_DIR}/${OUTPUT_ROOT_DIR}/libs/libtomcrypt.a"
)