set(target_name iotjs)
set(target_definitions) # define
set(target_sources "${CMAKE_SOURCE_DIR}/src/main.c") # *.c
set(target_libs) # library

set(EXECUTABLE_OUTPUT_PATH "../bin")
include(${CMAKE_SOURCE_DIR}/cmake/main.cmake)