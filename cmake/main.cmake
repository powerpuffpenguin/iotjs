message("Begin cmake ${target_name}")
project(${target_name})
list(APPEND target_definitions
    "-DVM_IOTJS_OS=\"${VM_IOTJS_OS}\""
    "-DVM_IOTJS_ARCH=\"${VM_IOTJS_ARCH}\""
)
list(APPEND target_libs
    -lm
)

if(LINK_STATIC_GLIC)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()
set(CMAKE_C_FLAGS "-std=c99")

include(${CMAKE_SOURCE_DIR}/cmake/third_party.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/source.cmake)


add_definitions(${target_definitions})
if(build_test)
    add_executable(${target_name} ${target_sources})
else()
    set(sources)
    foreach(source ${target_sources})
        if(NOT ${source} MATCHES ".*_test.c")
            list(APPEND sources ${source})
        endif()
    endforeach()
    add_executable(${target_name} ${sources})
endif()
target_link_libraries(${target_name} ${target_libs})


