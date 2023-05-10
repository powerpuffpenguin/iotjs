set(target_name iotjs_test)
set(target_definitions) # define
set(target_sources "${CMAKE_SOURCE_DIR}/test/main.c") # *.c
set(target_libs) # library

set(build_test TRUE)
set(EXECUTABLE_OUTPUT_PATH "../bin")
include(${CMAKE_SOURCE_DIR}/cmake/main.cmake)