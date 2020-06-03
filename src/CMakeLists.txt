
# runner-common sources
SET(RUNNER_COMMON_SRC runner-common.cpp)

execute_process(COMMAND git rev-parse HEAD
    OUTPUT_VARIABLE FUZZER_COMMIT_ID
    COMMAND_ECHO STDOUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(INFO "Commit ID : ${FUZZER_COMMIT_ID}")

add_compile_definitions(FUZZER_COMMIT="${FUZZER_COMMIT_ID}")
add_compile_definitions(V8_COMPRESS_POINTERS)
# runner-spidermonkey executable
add_executable(runner-spidermonkey ${RUNNER_COMMON_SRC} runner-spidermonkey.cpp)
add_compile_definitions(DEBUG)
add_dependencies(runner-spidermonkey build-v8 )
target_include_directories(runner-spidermonkey 
    PUBLIC "${CMAKE_BINARY_DIR}/third-party/spidermonkey/dist/include"
)
target_link_libraries(runner-spidermonkey
    -lmozjs-${MOZ_VERSION}
)
target_link_directories(runner-spidermonkey
    PRIVATE ${CMAKE_BINARY_DIR}/third-party/spidermonkey/dist/bin
)

# runner-v8 executable
add_executable(runner-v8 ${RUNNER_COMMON_SRC} runner-v8.cpp)
add_dependencies(runner-v8 build-v8)
target_include_directories(runner-v8 
    PUBLIC "${CMAKE_SOURCE_DIR}/third-party/v8/include"
)
target_link_libraries(runner-v8 
    -lv8_ext_diff_fuzz 
    -lv8_libplatform
)
target_link_directories(runner-v8 
    PUBLIC ${CMAKE_BINARY_DIR}/third-party/v8
)

# wasm-gen executable
add_executable(wasm-gen ${RUNNER_COMMON_SRC} wasm-generator.cpp)
add_dependencies(wasm-gen build-v8)
target_include_directories(wasm-gen 
    PUBLIC "${CMAKE_SOURCE_DIR}/third-party/v8/include"
)
target_link_libraries(wasm-gen 
    -lv8_ext_diff_fuzz 
    -lv8_libplatform
)
target_link_directories(wasm-gen 
    PUBLIC ${CMAKE_BINARY_DIR}/third-party/v8
)

# random-gen
add_executable(random-gen ${RUNNER_COMMON_SRC} random-gen.cpp)
add_dependencies(random-gen build-v8)
target_include_directories(random-gen 
    PUBLIC "${CMAKE_SOURCE_DIR}/third-party/v8/include"
)
target_link_libraries(random-gen 
    -lv8_ext_diff_fuzz 
    -lv8_libplatform
)
target_link_directories(random-gen 
    PUBLIC ${CMAKE_BINARY_DIR}/third-party/v8
)

# runner-coordinator.cpp
add_executable(runner-coordinator 
    ${RUNNER_COMMON_SRC} 
    runner-coordinator.cpp
    fuzzer-db.cpp
)
target_include_directories(runner-coordinator
    PUBLIC "${CMAKE_SOURCE_DIR}/third-party/quince/include"
    PUBLIC "${CMAKE_SOURCE_DIR}/third-party/quince-sqlite/include"
)
target_link_libraries(runner-coordinator
    quince
    quince-sqlite
    sqlite3
    boost_date_time
    boost_filesystem
    boost_thread
    pthread
)
target_link_directories(runner-coordinator
    PUBLIC ${CMAKE_BINARY_DIR}/lib
)
# target_compile_options(runner-coordinator PUBLIC -E)
