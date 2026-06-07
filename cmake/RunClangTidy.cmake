if(NOT EXISTS "${COMPILE_COMMANDS}")
    message(FATAL_ERROR "Compilation database not found: ${COMPILE_COMMANDS}")
endif()

file(REAL_PATH "${SOURCE_DIR}" SOURCE_DIR_REAL)
file(READ "${COMPILE_COMMANDS}" COMPILE_DATABASE)
string(JSON COMMAND_COUNT LENGTH "${COMPILE_DATABASE}")

set(TIDY_C_FILES)
set(TIDY_CXX_FILES)
if(COMMAND_COUNT GREATER 0)
    math(EXPR LAST_COMMAND "${COMMAND_COUNT} - 1")
    foreach(INDEX RANGE ${LAST_COMMAND})
        string(JSON COMMAND_DIRECTORY GET "${COMPILE_DATABASE}" ${INDEX} directory)
        string(JSON COMMAND_FILE GET "${COMPILE_DATABASE}" ${INDEX} file)

        cmake_path(ABSOLUTE_PATH COMMAND_FILE
            BASE_DIRECTORY "${COMMAND_DIRECTORY}"
            NORMALIZE
            OUTPUT_VARIABLE COMMAND_FILE_ABSOLUTE
        )
        file(REAL_PATH "${COMMAND_FILE_ABSOLUTE}" COMMAND_FILE_REAL)
        cmake_path(IS_PREFIX SOURCE_DIR_REAL "${COMMAND_FILE_REAL}" NORMALIZE IS_PROJECT_SOURCE)

        if(IS_PROJECT_SOURCE AND COMMAND_FILE_REAL MATCHES "\\.c$")
            list(APPEND TIDY_C_FILES "${COMMAND_FILE_REAL}")
        elseif(IS_PROJECT_SOURCE AND COMMAND_FILE_REAL MATCHES "\\.(cc|cpp|cxx)$")
            list(APPEND TIDY_CXX_FILES "${COMMAND_FILE_REAL}")
        endif()
    endforeach()
endif()

list(REMOVE_DUPLICATES TIDY_C_FILES)
list(REMOVE_DUPLICATES TIDY_CXX_FILES)
list(SORT TIDY_C_FILES)
list(SORT TIDY_CXX_FILES)

if(NOT TIDY_C_FILES AND NOT TIDY_CXX_FILES)
    message(FATAL_ERROR "No translation units under ${SOURCE_DIR_REAL} found in ${COMPILE_COMMANDS}")
endif()

list(LENGTH TIDY_C_FILES TIDY_C_FILE_COUNT)
list(LENGTH TIDY_CXX_FILES TIDY_CXX_FILE_COUNT)
math(EXPR TIDY_FILE_COUNT "${TIDY_C_FILE_COUNT} + ${TIDY_CXX_FILE_COUNT}")
message(STATUS "Running clang-tidy on ${TIDY_FILE_COUNT} files under ${SOURCE_DIR_REAL}")

set(CLANG_TIDY_COMMON_ARGS
    -p "${COMPILE_COMMANDS}"
    --extra-arg-before=--target=arm-none-eabi
)
if(NEWLIB_INCLUDE_DIR)
    list(APPEND CLANG_TIDY_COMMON_ARGS "--extra-arg=-isystem${NEWLIB_INCLUDE_DIR}")
endif()

if(TIDY_C_FILES)
    execute_process(
        COMMAND "${CLANG_TIDY_EXECUTABLE}" ${CLANG_TIDY_COMMON_ARGS} ${TIDY_C_FILES}
        COMMAND_ECHO STDOUT
        RESULT_VARIABLE CLANG_TIDY_C_RESULT
    )
    if(NOT CLANG_TIDY_C_RESULT EQUAL 0)
        message(FATAL_ERROR "clang-tidy failed for C sources with exit code ${CLANG_TIDY_C_RESULT}")
    endif()
endif()

if(TIDY_CXX_FILES)
    set(CLANG_TIDY_CXX_ARGS
        ${CLANG_TIDY_COMMON_ARGS}
        --extra-arg-before=-nostdinc++
    )
    foreach(INCLUDE_DIR IN LISTS GCC_INCLUDE_DIRS)
        list(APPEND CLANG_TIDY_CXX_ARGS "--extra-arg=-isystem${INCLUDE_DIR}")
    endforeach()

    execute_process(
        COMMAND "${CLANG_TIDY_EXECUTABLE}" ${CLANG_TIDY_CXX_ARGS} ${TIDY_CXX_FILES}
        COMMAND_ECHO STDOUT
        RESULT_VARIABLE CLANG_TIDY_CXX_RESULT
    )
    if(NOT CLANG_TIDY_CXX_RESULT EQUAL 0)
        message(FATAL_ERROR "clang-tidy failed for C++ sources with exit code ${CLANG_TIDY_CXX_RESULT}")
    endif()
endif()
