message(STATUS "third party: http-parser-2.9.4")

set(dir "${CMAKE_SOURCE_DIR}/third_party/http-parser-2.9.4")
include_directories(${dir})

list(APPEND target_sources
    "${dir}/http_parser.c"
)