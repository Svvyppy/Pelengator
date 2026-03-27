cmake_minimum_required(VERSION 3.22)
project(PelengHostTests LANGUAGES C CXX)

include(FetchContent)

enable_testing()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMSISCORE "${CMAKE_CURRENT_LIST_DIR}/../Drivers/CMSIS" CACHE PATH "CMSIS Core source root" FORCE)

FetchContent_Declare(
    cmsis_dsp
    GIT_REPOSITORY https://github.com/ARM-software/CMSIS-DSP.git
    GIT_TAG e968b3cc3fdf5f2dec242977794e351ed4a42bce
    GIT_SHALLOW TRUE
)
FetchContent_Populate(cmsis_dsp)

set(HOST ON CACHE BOOL "Build CMSIS-DSP for host" FORCE)
set(NEON OFF CACHE BOOL "" FORCE)
set(NEONEXPERIMENTAL OFF CACHE BOOL "" FORCE)
set(HELIUMEXPERIMENTAL OFF CACHE BOOL "" FORCE)
set(LOOPUNROLL OFF CACHE BOOL "" FORCE)
set(ROUNDING OFF CACHE BOOL "" FORCE)
set(MATRIXCHECK OFF CACHE BOOL "" FORCE)
set(HELIUM OFF CACHE BOOL "" FORCE)
set(MVEF OFF CACHE BOOL "" FORCE)
set(MVEI OFF CACHE BOOL "" FORCE)
set(MVEFLOAT16 OFF CACHE BOOL "" FORCE)
set(DISABLEFLOAT16 ON CACHE BOOL "" FORCE)
set(AUTOVECTORIZE OFF CACHE BOOL "" FORCE)
set(LAXVECTORCONVERSIONS ON CACHE BOOL "" FORCE)

add_subdirectory(${cmsis_dsp_SOURCE_DIR}/Source ${cmsis_dsp_BINARY_DIR}/CMSISDSP)

add_executable(peleng_host_tests
    host_tests.cpp
    ../src/peleng/Filter.cpp
    ../src/peleng/DelayEstimator.cpp
)

target_include_directories(peleng_host_tests PRIVATE
    ../src/peleng
    ../src
)

target_link_libraries(peleng_host_tests PRIVATE CMSISDSP)

target_compile_options(peleng_host_tests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME peleng_host_tests COMMAND peleng_host_tests)
