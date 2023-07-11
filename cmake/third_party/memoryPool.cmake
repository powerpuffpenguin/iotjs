message(STATUS "third party: memoryPool")

set(dir "${CMAKE_SOURCE_DIR}/third_party/memoryPool")
include_directories(${dir})

list(APPEND target_sources
    "${dir}/mem_pool.c"
    "${dir}/rbTree.c"
)