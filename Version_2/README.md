# Advanced Task Manager (Refactored)

A Raylib-based Task Manager refactored from a monolithic 800-line source file into a
professionally structured, modular C11 project following the guide in
`C_TaskManager_Refactoring_Guide.docx`.

## Directory Structure

```
task_manager/
├── CMakeLists.txt
├── README.md
├── .clang-format
├── include/                # Public headers (API boundary)
│   ├── tm_types.h          # All shared types & result codes
│   ├── tm_process.h
│   ├── tm_perf.h
│   ├── tm_startup.h
│   ├── tm_ui.h
│   ├── tm_platform.h
│   └── tm_log.h
└── src/
    ├── main.c              # Entry point only (~30 lines)
    ├── core/               # Business logic (no Raylib)
    │   ├── tm_process.c
    │   ├── tm_perf.c
    │   ├── tm_startup.c
    │   └── tm_app_history.c
    ├── ui/                 # All Raylib rendering
    │   ├── ui_core.c
    │   ├── ui_theme.c
    │   ├── ui_button.c
    │   ├── ui_scrollbar.c
    │   ├── ui_tab_processes.c
    │   ├── ui_tab_performance.c
    │   ├── ui_tab_history.c
    │   └── ui_tab_startup.c
    ├── platform/           # OS-specific adapters
    │   ├── platform_posix.c
    │   └── platform_win32.c
    └── utils/
        └── tm_log.c
```

## Building

### Prerequisites
- CMake >= 3.16
- GCC / Clang / MSVC (C11)
- Raylib 4.x or 5.x (fetched automatically if not found)

### Linux / macOS
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/task_manager
```

### Windows (MSVC)
```bat
cmake -B build
cmake --build build --config Release
.\build\Release\task_manager.exe
```

## Key Architecture Decisions

| Pattern            | Where applied                          |
|--------------------|----------------------------------------|
| **Strategy**       | Tab dispatcher (`TmTabDescriptor[]`)   |
| **Command**        | Button actions (`TmCommandFn`)         |
| **Observer**       | Process list refresh callbacks         |
| **Platform Adapter** | `TmPlatform` vtable (POSIX / Win32)  |

## Keyboard Shortcuts
- **F5** — Refresh process list
- **Delete** — End selected process
- **E** — Enable selected startup app
- **D** — Disable selected startup app

## Refactoring Status

| Phase | Description                 | Status     |
|-------|-----------------------------|------------|
| 1     | Audit & Baseline Metrics    | Planned    |
| 2     | Define Types & Constants    | Completed  |
| 3     | File Reorganisation         | ✅ Completed |
| 4     | Extract Platform Layer      | ✅ Completed |
| 5     | Refactor Core Logic         | ✅ Completed |
| 6     | Apply Design Patterns       | ✅ Completed |
| 7     | Error Handling & Robustness | Planned    |
| 8     | Polish, Docs & CI           | Planned    |
