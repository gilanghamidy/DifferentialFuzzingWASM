cmake_minimum_required(VERSION 3.16)

project(DifferentialFuzzingWASM VERSION 0.1)

# Requirements

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Execute git submodule update
message(STATUS "Refreshing git submodule")
execute_process(COMMAND git submodule update             
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Build the buildscript for SpiderMonkey
message(STATUS "Generate Build for SpiderMonkey")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/third-party/spidermonkey)
execute_process(COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/third-party/gecko-dev/js/src/configure.in --enable-debug --disable-tests --disable-gtest-in-build --disable-optimize JS_STANDALONE --disable-jemalloc
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/third-party/spidermonkey
    RESULT_VARIABLE RES_MOZ_GEN
    OUTPUT_FILE "/proc/self/fd/0")

# Read SpiderMonkey version
file(READ ${CMAKE_BINARY_DIR}/third-party/spidermonkey/binaries.json MOZ_BINARIES_JSON)
string(REGEX MATCHALL "libmozjs-([A-Za-z0-9]+)\\.so" MOZ_VERSIONS ${MOZ_BINARIES_JSON})
list(GET MOZ_VERSIONS 0 MOZ_VERSION)
string(REGEX REPLACE "libmozjs-([A-Za-z0-9]+)\\.so" "\\1" MOZ_VERSION ${MOZ_VERSION})
message(INFO "Moz Version: ${MOZ_VERSION}")

# Target for SpiderMonkey
add_custom_target(build-moz make -j$$(nproc)
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/third-party/spidermonkey
    USES_TERMINAL)

# Build the buildscript for V8
message(STATUS "Generate Build for V8")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/third-party/v8)

set(V8_GEN_ARGS "is_debug=true target_cpu=\"x64\" v8_target_cpu=\"x64\" v8_enable_disassembler=true v8_expose_symbols=true")

# Get relative path
execute_process(COMMAND python -c "import os.path; print os.path.relpath(\"${CMAKE_BINARY_DIR}/third-party/v8\", \"${CMAKE_SOURCE_DIR}/third-party/v8\")"
    OUTPUT_VARIABLE V8_BUILD_PATH
    COMMAND_ECHO STDOUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(INFO "Path: ${V8_BUILD_PATH}" )

execute_process(COMMAND ${CMAKE_SOURCE_DIR}/third-party/depot_tools/gn gen ${V8_BUILD_PATH} --args=${V8_GEN_ARGS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/third-party/v8
    COMMAND_ECHO STDOUT
    RESULT_VARIABLE RES_MOZ_GEN
    OUTPUT_FILE "/proc/self/fd/0")

    # Target for V8
add_custom_target(build-v8 ninja
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/third-party/v8
    DEPENDS build-moz # Ordering the build
    USES_TERMINAL)


if(${RES_MOZ_GEN})
    message(FATAL_ERROR "Something wrong with generating build for SpiderMonkey: " ${RES_MOZ_GEN} )
endif()

add_subdirectory (src)