add_executable(runner-spidermonkey runner-spidermonkey.cpp)

add_compile_definitions(DEBUG)

add_dependencies(runner-spidermonkey
    build-moz
)

target_include_directories(runner-spidermonkey 
    PUBLIC "${CMAKE_BINARY_DIR}/third-party/spidermonkey/dist/include"
)

target_link_libraries(runner-spidermonkey
    -lmozjs-${MOZ_VERSION}
)

target_link_directories(runner-spidermonkey
    PRIVATE ${CMAKE_BINARY_DIR}/third-party/spidermonkey/dist/bin
)