# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Junbo Zheng

# Unix port: message-queue transport (both ends) + a CLI that requests
# level changes in a running application.

add_library(logmask_unix STATIC src/mq.c)
target_include_directories(logmask_unix
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(logmask_unix PUBLIC logmask)
find_library(RT_LIB rt)
if(RT_LIB)
    target_link_libraries(logmask_unix PUBLIC ${RT_LIB})
endif()

# the CLI is a pure request tool: it links only the transport -- the
# static library pulls in nothing it does not reference
add_executable(logmask_cli src/cli.c)
target_link_libraries(logmask_cli PRIVATE logmask_unix)
