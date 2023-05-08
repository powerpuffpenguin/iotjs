message(STATUS "third party: duktape-2.7.0")

set(dir "${CMAKE_SOURCE_DIR}/third_party/duktape-2.7.0")
include_directories(${dir})

file(GLOB  files 
    "${dir}/*.c"
)
list(APPEND target_sources
    ${files}
)