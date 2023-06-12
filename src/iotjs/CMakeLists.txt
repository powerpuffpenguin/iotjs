set(target_name libiotjs)
set(target_definitions) # define
set(target_sources) # *.c
set(target_libs) # library

set(build_lib TRUE)
set(LIBRARY_OUTPUT_PATH "../../../libs")
set(CMAKE_STATIC_LIBRARY_PREFIX "")
include(${CMAKE_SOURCE_DIR}/cmake/main.cmake)