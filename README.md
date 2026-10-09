# logmask

[![License: Apache-2.0](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)

Runtime per-module log level filtering, as a small portable C library.

Most logging systems filter by level globally or per-task. logmask adds the
missing axis: **per module**, changeable at runtime. Each source directory
declares which module it belongs to, and every log line first checks the
level mask of its own module — so you can turn one subsystem up to debug
while everything else stays quiet, without rebuilding or restarting.

```c
#define LOGMASK_LOG_MODULE LOGMASK_MOD_BT   /* this file logs as "bt" */
#include <logmask.h>

LOG_INF(bt, "link up\n");     /* passes when bt's mask allows INFO */
LOG_DBG(bt, "len=%d\n", 42); /* silenced until bt is turned to debug */
```

```text
$ logmask_cli bt debug    # the unix port's tool: one module to debug,
                          # everything else untouched
module    mask  level          /* in-process: logmask_show() */
--------  ----  -------
default   0xff  debug
bt        0x7f  info
...
```

Levels are the POSIX syslog names, and the default output backend is
syslog(3) itself, so on unix/NuttX the lines land under the process's
normal syslog filtering with zero configuration. The backend is pluggable
(`LOG_BACKEND`) for platforms without syslog.h; all platform-specific code
(IPC transports, request tools) lives in [`ports/`](ports/) — the core is
pure logic with zero system calls.

## How it works

A log line passes through a chain of gates, and each level can only be
*more* restrictive than the ones behind it:

```
g_logmask[]           runtime per-module mask, changed by set_level
  -> your backend     whatever LOG_BACKEND forwards to
```

The split that makes this cheap:

- **Compile time decides which slot to check.** `-DLOGMASK_LOG_MODULE=LOGMASK_MOD_BT`
  makes every file in that directory index an enum constant, which the
  compiler folds into a plain array access — no string compares, no lookup,
  per log line.
- **Runtime decides what is in the slot.** `g_logmask[]` holds one level
  bitmap per module. `logmask_set_level()` changes it; a reboot (or
  `logmask_set_mask(mod, LOGMASK_MASK_DEFAULT)`) resets it.

The module list is a single X-macro (`LOGMASK_MODULE_LIST`) that the
consumer defines — the enum and the name table are both generated from it,
so they can never drift apart, and the library source never needs editing.
Levels use the syslog numbering, and the mask encoding matches
`LOG_MASK`/`LOG_UPTO`/`LOG_ALL` from syslog.h, so on a POSIX platform
`g_logmask[mod] = LOG_UPTO(LOG_INFO)` is valid directly.

## Building

Any CMake ≥ 3.27, or just compile the sources and put `include/` on the
include path: `src/logmask.c` is the whole core — no OS calls, no IPC.

```sh
cmake -B build -DLOGMASK_BUILD_EXAMPLES=ON -DLOGMASK_BUILD_UNIX_PORT=ON
cmake --build build
./build/logmask_example    # ~1 minute tour of the API
```

## Usage

```c
#include <logmask.h>

/* per-module runtime control */
logmask_show();                        /* list modules and levels */
logmask_set_level("bt", "debug");      /* by name + letter or word */
logmask_set_level("all", "o");         /* silence everything */
logmask_set_mask(LOGMASK_MOD_BT, LOG_UPTO(LOG_WARNING));
```

Level letters: `d` DEBUG, `i` INFO, `n` NOTICE, `w` WARNING, `e` ERROR,
`c` CRITICAL, `a` ALERT, `r` EMERG, `o` OFF (full words also accepted).

`LOGMASK_FUNC_NAMES` adds `__func__` to each log line.

### Assigning a module

- **Single file**: `#define LOGMASK_LOG_MODULE LOGMASK_MOD_BT` before the
  first `#include` (a late `#define` lands after the fallback has already
  locked the file to the default module).
- **Whole directory**: pass `-DLOGMASK_LOG_MODULE=LOGMASK_MOD_BT` for your
  build system / directory. Directory-level and file-level declarations
  must not be mixed.

### Backends

Three tiers, first match wins:

1. **Your own** -- define `LOG_BACKEND` before including the header:

   ```c
   #define LOG_BACKEND(level, fmt, ...) \
       my_log(level, fmt, ##__VA_ARGS__)   /* UART, ESP_LOG, ... */
   #include <logmask.h>
   ```

2. **Forced stdio** -- define `LOGMASK_BACKEND_PRINTF` (plain `printf`,
   level dropped; what the examples use so their output is on stdout).

3. **Auto** -- `syslog(3)` when the platform has `syslog.h`, `printf`
   otherwise. Zero configuration on unix and on simple hosts alike.

### Cross-task changes (port-owned transport)

Level changes from outside the process go through a transport, and
transports are port-owned: the port defines the wire format and the queue
mechanics, and its receive side applies commands via the core's public
`logmask_set_level()`. The core has no IPC symbols.

The unix port (`-DLOGMASK_BUILD_UNIX_PORT=ON`) routes them over a POSIX
message queue (`logmask_unix.h`): the command side calls
`logmask_mq_send()`, the application's event loop receives via
`logmask_mq_open()` / `logmask_mq_process(fd)`. Working pair:
`ports/unix/src/cli.c` sends, `examples/server.c` receives -- run one of
each and try `logmask_cli ap debug`.

The port library's own log lines ("applied" / unknown-module warnings) use
the backend it was compiled with (syslog by default), not the
application's — `LOG_BACKEND` is a per-translation-unit compile-time macro.

## Ports

| Port | Provides |
|---|---|
| [`ports/unix`](ports/unix/) | `logmask_unix` -- message-queue transport (`logmask_mq_open/send/process`) + `logmask_cli`, a shell tool that requests level changes in a running application |

## Defining your modules

The list must reach **every translation unit that logs, including the
logmask library itself** (the enum and the mask array live in logmask.c).
Two equivalent ways, both demonstrated in `examples/`:

**A conf header** (`examples/config.h` + `examples/simple.c`) — include it
before `<logmask.h>` everywhere, or force-include it for the whole target
(see the root `CMakeLists.txt`):

```c
/* my_config.h */
#define LOGMASK_MODULE_LIST \
    X(DEFAULT, default) X(BT, bt) X(UI, ui) X(NEWONE, newone)
```

**A directory-scope compile definition**, set *before*
`add_subdirectory(logmask)` so the library sees it too:

```cmake
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
    "LOGMASK_MODULE_LIST=X(DEFAULT,default) X(BT,bt) X(UI,ui)")
add_subdirectory(logmask)
```

Names are bare tokens, no quotes. Keep `X(DEFAULT, default)` first: files
that declare no module fall back to it. Every entry becomes
`LOGMASK_MOD_<ID>`, settable by name at runtime. A per-file `#define`
before the first `#include <logmask.h>` works too, but only if the rest of
the build already agrees on the same list.

The level constants are the POSIX syslog names (`LOG_INFO`, `LOG_DEBUG`,
...) with the standard numbering, so code written against syslog reads
naturally; when syslog.h is also included the identical definitions merge
silently. The mask encoding matches `LOG_UPTO`/`LOG_ALL` the same way.

## License

[Apache-2.0](LICENSE)
