cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_PROJECT_NAME Peleng)
option(PELENG_FETCH_DEPS "Download CMSIS-DSP and HAL/LL during configure" OFF)
option(PELENG_ENABLE_CLANG_TIDY "Add the clang-tidy static analysis target" ON)
set(BOARD "devboard" CACHE STRING "Target board platform")
set_property(CACHE BOARD PROPERTY STRINGS devboard)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME} C ASM CXX)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

include(FetchContent)

if(PELENG_FETCH_DEPS)
    set(PELENG_CMSIS_DSP_REPOSITORY "https://github.com/ARM-software/CMSIS-DSP.git")
    set(PELENG_CMSIS_DSP_TAG "e968b3cc3fdf5f2dec242977794e351ed4a42bce")
    set(PELENG_HAL_REPOSITORY "https://github.com/STMicroelectronics/stm32g4xx-hal-driver.git")
    set(PELENG_HAL_TAG "7186efe25f233560d6859c18b1c12f4e821d2ba7")

    FetchContent_Declare(cmsis_dsp
        GIT_REPOSITORY ${PELENG_CMSIS_DSP_REPOSITORY}
        GIT_TAG ${PELENG_CMSIS_DSP_TAG}
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR Source
    )
    FetchContent_MakeAvailable(cmsis_dsp)
    set(PELENG_CMSIS_DSP_DIR "${cmsis_dsp_SOURCE_DIR}" CACHE PATH "CMSIS-DSP source root" FORCE)

    FetchContent_Declare(stm32_hal
        GIT_REPOSITORY ${PELENG_HAL_REPOSITORY}
        GIT_TAG ${PELENG_HAL_TAG}
        GIT_SHALLOW TRUE
    )
    FetchContent_Populate(stm32_hal)
    set(PELENG_HAL_DRIVER_DIR "${stm32_hal_SOURCE_DIR}" CACHE PATH "STM32 HAL driver source root" FORCE)
else()
    set(PELENG_CMSIS_DSP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS-DSP" CACHE PATH "CMSIS-DSP source root" FORCE)
    set(PELENG_HAL_DRIVER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32G4xx_HAL_Driver" CACHE PATH "STM32 HAL driver source root" FORCE)
    add_subdirectory("${PELENG_CMSIS_DSP_DIR}/Source" bin_dsp)
endif()

if(NOT EXISTS "${PELENG_CMSIS_DSP_DIR}/Source/CMakeLists.txt")
    message(FATAL_ERROR "CMSIS-DSP source tree not found at ${PELENG_CMSIS_DSP_DIR}")
endif()
if(NOT EXISTS "${PELENG_HAL_DRIVER_DIR}/Src/stm32g4xx_hal.c")
    message(FATAL_ERROR "STM32 HAL driver source tree not found at ${PELENG_HAL_DRIVER_DIR}")
endif()

set(CMSIS_CORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS")
set(CMSISCORE "${CMSIS_CORE_ROOT}" CACHE PATH "CMSIS Core source root" FORCE)

if(NOT TARGET CMSISDSP)
    message(FATAL_ERROR "CMSISDSP target was not created")
endif()

add_executable(${CMAKE_PROJECT_NAME})
add_subdirectory(src)

if(PELENG_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy-20)
    if(CLANG_TIDY_EXECUTABLE)
        find_path(CLANG_TIDY_NEWLIB_INCLUDE_DIR
            NAMES errno.h
            PATHS /usr/include/newlib
        )
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -dumpfullversion
            OUTPUT_VARIABLE CLANG_TIDY_GCC_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=include
            OUTPUT_VARIABLE CLANG_TIDY_GCC_INCLUDE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(CLANG_TIDY_GCC_INCLUDE_DIRS
            "${CLANG_TIDY_NEWLIB_INCLUDE_DIR}/c++/${CLANG_TIDY_GCC_VERSION}"
            "${CLANG_TIDY_NEWLIB_INCLUDE_DIR}/c++/${CLANG_TIDY_GCC_VERSION}/arm-none-eabi"
            "${CLANG_TIDY_NEWLIB_INCLUDE_DIR}/c++/${CLANG_TIDY_GCC_VERSION}/backward"
            "${CLANG_TIDY_GCC_INCLUDE_DIR}"
            "${CLANG_TIDY_GCC_INCLUDE_DIR}/../include-fixed"
            "${CLANG_TIDY_NEWLIB_INCLUDE_DIR}"
        )

        add_custom_target(clang-tidy
            COMMAND ${CMAKE_COMMAND}
                -DCLANG_TIDY_EXECUTABLE=${CLANG_TIDY_EXECUTABLE}
                -DCOMPILE_COMMANDS=${CMAKE_BINARY_DIR}/compile_commands.json
                -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/src
                -DNEWLIB_INCLUDE_DIR=${CLANG_TIDY_NEWLIB_INCLUDE_DIR}
                "-DGCC_INCLUDE_DIRS=${CLANG_TIDY_GCC_INCLUDE_DIRS}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunClangTidy.cmake
            COMMENT "Running clang-tidy on sources under src/"
            VERBATIM
        )
    else()
        message(WARNING "clang-tidy-20 not found: 'clang-tidy' target is unavailable")
    endif()
endif()

# CMSIS core headers are still provided locally.
target_include_directories(CMSISDSP PRIVATE
    ${CMSIS_CORE_ROOT}/Include
)

# The real-time DSP path must stay optimized even in Debug firmware; otherwise
# the half-buffer processing time is dominated by unoptimized CMSIS-DSP code.
target_compile_options(CMSISDSP PRIVATE
    -O3
    -ffast-math
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${PELENG_CMSIS_DSP_DIR}/Include
    ${CMSIS_CORE_ROOT}/Include
)

# Remove wrong libob.a library dependency when using cpp files.
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    CMSISDSP
)

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/docs/doxygen")
    set(DOXYGEN_IN "${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in")
    set(DOXYGEN_OUT "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")

    file(MAKE_DIRECTORY "${DOXYGEN_OUTPUT_DIR}")

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${DOXYGEN_OUTPUT_DIR}/html"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DOXYGEN_OUTPUT_DIR}"
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
else()
    message(STATUS "Doxygen not found: 'docs' target is unavailable")
endif()
