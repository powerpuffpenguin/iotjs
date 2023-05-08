cmake_minimum_required(VERSION 3.0)

# 定義項目
set(target_name iotjs) # 項目名稱
set(target_definitions) # define
set(target_sources) # *.c
set(target_libs) # library

list(APPEND target_libs
    -lm
)

project(${target_name})

set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_C_FLAGS "-std=c99")

include(cmake/third_party.cmake)
include(cmake/source.cmake)

add_definitions(${target_definitions})
add_executable(${target_name} ${target_sources})
target_link_libraries(${target_name} ${target_libs})

