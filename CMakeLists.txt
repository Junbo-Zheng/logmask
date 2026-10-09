# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Junbo Zheng

cmake_minimum_required(VERSION 3.27)
project(logmask C)

set(CMAKE_C_STANDARD 11)

# emit build/compile_commands.json for clangd / IDE code intelligence
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(logmask STATIC src/logmask.c)
target_include_directories(logmask
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

if(LOGMASK_BUILD_EXAMPLES)
    # The example is a self-contained mini-application: it compiles
    # logmask.c into itself and defines its own module taxonomy, which is
    # exactly the integration pattern a real application uses (the list
    # must reach every TU that logs, logmask.c included -- hence the
    # target-wide definition instead of a #define in one .c file).
    add_executable(logmask_example examples/simple.c src/logmask.c)
    target_include_directories(logmask_example
        PRIVATE include ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    # force-include the conf so src/logmask.c sees the same module list
    # (simple.c includes it explicitly as the visible pattern)
    target_compile_definitions(logmask_example PRIVATE LOGMASK_BACKEND_PRINTF)
    target_compile_options(logmask_example
        PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/examples/config.h)
    set_target_properties(logmask_example PROPERTIES
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

if(LOGMASK_BUILD_EXAMPLES AND LOGMASK_BUILD_UNIX_PORT)
    # the receiving side of the CLI demo: same self-contained pattern as
    # logmask_example (own taxonomy via force-include, logmask.c compiled
    # in), plus the unix port's transport linked in
    add_executable(logmask_server examples/server.c src/logmask.c)
    target_link_libraries(logmask_server PRIVATE logmask_unix)
    target_include_directories(logmask_server
        PRIVATE include ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    target_compile_definitions(logmask_server PRIVATE LOGMASK_BACKEND_PRINTF)
    target_compile_options(logmask_server
        PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/examples/config.h)
    set_target_properties(logmask_server PROPERTIES
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

if(LOGMASK_BUILD_UNIX_PORT)
    add_subdirectory(ports/unix)
endif()

# Drop-in consumption: install headers + the library so consumers can
#   add_subdirectory(logmask) + target_link_libraries(app PRIVATE logmask)
# or
#   cmake --install <build> --prefix <prefix>
# and use find_package-style include paths.
# Skipped in example builds: the example bakes its demo module list into
# this library instance, which must not be what gets installed.
if(NOT LOGMASK_BUILD_EXAMPLES)
    include(GNUInstallDirs)
    install(TARGETS logmask ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
