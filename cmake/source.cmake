include_directories(src/core)
set(dirs
    src/core
    src
)
foreach(dir ${dirs})
    include_directories(${dir})
    file(GLOB  files 
        "${dir}/*.c"
    )
    list(APPEND target_sources
        ${files}
    )
endforeach()
