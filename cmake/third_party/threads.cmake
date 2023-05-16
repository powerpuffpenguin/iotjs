message(STATUS "third party: C-Thread-Pool")

set(dir "${CMAKE_SOURCE_DIR}/third_party/C-Thread-Pool")
include_directories(${dir})

file(GLOB  files 
    "${dir}/*.c"
)
list(APPEND target_sources
    ${files}
)