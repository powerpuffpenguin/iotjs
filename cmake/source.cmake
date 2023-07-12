include_directories(src/core)
set(dirs
    src/iotjs/core
    src/iotjs/assert
    src/iotjs/modules
    src/iotjs/mempool
)
include_directories("${CMAKE_SOURCE_DIR}/src")
foreach(dir ${dirs})
    file(GLOB  files 
        "${CMAKE_SOURCE_DIR}/${dir}/*.c"
    )
    list(APPEND target_sources
        "${files}"
    )
endforeach()
