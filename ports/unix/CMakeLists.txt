# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Junbo Zheng

# Unix port: syslog backend header + a CLI that requests level changes in
# a running application over the POSIX message queue.

# the CLI is a pure request tool: it links only the client transport,
# not the mask array / name table
add_executable(logmask_cli src/logmask_cli.c ${CMAKE_SOURCE_DIR}/src/client.c)
target_include_directories(logmask_cli PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(logmask_cli PRIVATE LOGMASK_USE_MQ)
