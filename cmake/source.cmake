include_directories(src/core)
set(dirs
    src/iotjs/core
    src
)
include_directories(src)
foreach(dir ${dirs})
    file(GLOB  files 
        "${dir}/*.c"
    )
    list(APPEND target_sources
        ${files}
    )
endforeach()
