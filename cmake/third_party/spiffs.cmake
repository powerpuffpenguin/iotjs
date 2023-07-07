message(STATUS "third party: spiffs")

set(dir "${CMAKE_SOURCE_DIR}/third_party/spiffs/src")
include_directories(${dir})

file(GLOB  files 
    "${dir}/*.c"
)
list(APPEND target_sources
    ${files}
)