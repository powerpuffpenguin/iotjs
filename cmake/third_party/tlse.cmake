message(STATUS "third party: tlse")

set(dir "${CMAKE_SOURCE_DIR}/third_party/tlse")
include_directories(${dir})

file(GLOB  files 
    "${dir}/*.c"
)
list(APPEND target_sources
    ${files}
)