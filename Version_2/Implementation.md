# C Task Manager: Refactoring Report (ver1 → ver2)

**Course:** Advanced Programming Lab — 2nd Year CSE  
**Project:** C Task Manager (raylib)  
**Date:** February 2026

---

## Table of Contents

### Part A — AI Prompts Used
1. [Overview](#overview)
2. [Stage 1: Analysis & Planning Prompts](#stage-1-analysis--planning-prompts)
3. [Stage 2: Coding Convention & Standards Prompts](#stage-2-coding-convention--standards-prompts)
4. [Stage 3: Folder Structure Reorganization Prompts](#stage-3-folder-structure-reorganization-prompts)
5. [Stage 4: Code-Level Improvement Prompts](#stage-4-code-level-improvement-prompts)
6. [Stage 5: Documentation & Guide Prompts](#stage-5-documentation--guide-prompts)
7. [Prompt Design Philosophy](#prompt-design-philosophy)
8. [Summary of Prompt Categories](#summary-of-prompt-categories)

### Part B — Detailed Explanation of Changes
9. [Executive Summary](#9-executive-summary)
10. [Coding & Naming Conventions](#10-coding--naming-conventions)
11. [Design Model](#11-design-model)
12. [Design Patterns Analysis](#12-design-patterns-analysis)
13. [Detailed Change Log (ver1 → ver2)](#13-detailed-change-log-ver1--ver2)
14. [What Was Improved & What Remains](#14-what-was-improved--what-remains)
15. [Lessons Learned](#15-lessons-learned)
16. [Appendix A: File Metrics Comparison](#appendix-a-file-metrics-comparison)
17. [Appendix B: Module Dependency Map](#appendix-b-module-dependency-map)

---

# PART A — AI PROMPTS USED

---

## Overview

The C Task Manager project is a cross-platform system monitor built in C using the raylib graphics library. It clones the functionality of the Windows Task Manager, providing four tabs — Processes, Performance, App History, and Startup — each backed by linked list data structures and real OS process enumeration via `popen()`. The application supports resizable windows, interactive scrollbars, process termination, and startup app toggling.

Version 1 shipped as a single 800-line C source file containing all data structures, platform-specific `#ifdef _WIN32` blocks, UI rendering logic, business logic, and scroll bar math crammed together with no header files, no separation of concerns, and no error handling beyond silent `continue` on `malloc` failure. While functional, even a small change — say, adding a new tab or switching from simulated to real CPU data — required reading the entire file to understand the impact.

AI was used throughout the transformation from ver1 to ver2. We started with codebase analysis prompts to get a full inventory of what existed, then moved through naming convention design, folder structure planning, function decomposition, platform abstraction, and finally documentation generation. Each stage built on the previous one: the analysis output fed directly into the convention prompts, the convention decisions shaped the folder structure, and the structure decisions guided which functions needed to be split. The result is a 26-file, 6-directory modular project with zero `#ifdef` blocks outside the platform layer, a typed error system, three design patterns, and full Doxygen-style comments on every public function.

---

## Stage 1: Analysis & Planning Prompts

### Prompt 1.1 — Initial Codebase Audit

> "Analyze this C source file for a Task Manager application built with raylib. The file is approximately 800 lines long and is entirely self-contained. For every function, list its name, line number, parameter types, return type, and an estimate of its line count. For every global variable, list its name, type, and what it represents. For every struct and enum, list all fields/values and describe what the type models. Then identify: (1) functions that are doing more than one job, (2) magic numbers with no named constant, (3) global state that could be encapsulated, (4) platform-specific code mixed with portable code, (5) missing error handling on malloc() and popen(). Summarize the overall architecture in 3–5 sentences."

**Purpose:** Before touching any code we needed a complete inventory. Manually counting all 27 functions, 15 global variables, 7 structs, and 1 enum across 800 lines is tedious and error-prone. Asking AI to produce the inventory first gave us a shared baseline to reference in every subsequent prompt.

**What we learned:**
- The file had 27 functions, 15 module-level `static` globals, 7 struct types, and 1 enum (`TabType`) — all in a single translation unit with no headers
- `DrawPerformanceTab()` was the longest function at ~95 lines, drawing CPU, Memory, GPU, Disk, and System Info without any helper calls
- `GetProcessList()` mixed three completely different concerns: OS process enumeration (`popen` + `fgets`), linked list construction (`malloc` + pointer chaining), and UI state mutation (`endTaskBtn.enabled = false`)
- `InitializeApp()` initialized six unrelated subsystems (tabs, buttons, performance data, scrollbars, startup list, app history) in a single 65-line function with no sub-functions
- Magic numbers appeared 30+ times: `30` (row height), `45` (startup row height), `65` (history row height), `100` (history buffer size), `256` (name buffer), `1200`/`800` (window size), `8` (startup app count)

### Prompt 1.2 — Identifying Refactoring Opportunities

> "Based on the Task Manager codebase analysis, identify specific refactoring opportunities for each of the following categories. For each opportunity, reference the exact function or variable name from the code and explain concretely what the problem is and what the fix should be. Categories: (1) Functions that should be split — name each sub-function that should be created; (2) Global variables that should be moved into a struct — name the struct; (3) Magic numbers that need named constants — give the constant name and value; (4) Structs that need their own header file — name the header; (5) Platform-specific #ifdef blocks that should be isolated. Show before/after pseudocode for the two most impactful changes."

**Purpose:** The audit gave us a list of everything that existed. This prompt turned that list into an actionable refactoring plan. We needed to know not just "there are problems" but exactly which functions to split, what to name the pieces, and what headers to create.

**Key opportunities identified:**
- `DrawPerformanceTab()` should be split into `ui_tab_perf_draw_cpu()`, `ui_tab_perf_draw_memory()`, `ui_tab_perf_draw_gpu()`, `ui_tab_perf_draw_disk()`, `ui_tab_perf_draw_sysinfo()` — each independently replaceable when real OS APIs are added
- All 15 global variables (`processList`, `processCount`, `selectedProcessIndex`, `tabs[]`, `refreshBtn`, `message[]`, `perfData`, `screenWidth`, `screenHeight`, `processScrollBar`, etc.) should be collected into a single `TmAppState` struct and passed by pointer to every function
- `GetProcessList()` should be split into a platform layer (`open_process_list`, `parse_process_line`) and a core layer (`tm_process_list_refresh`) so that adding macOS support later only requires a new platform file
- The `#ifdef _WIN32` block inside `GetProcessList()` and `KillProcess()` should be moved to dedicated `platform_win32.c` and `platform_posix.c` files; main.c should select the adapter once at startup
- All magic numbers should become named constants in a shared `tm_types.h`: `TM_ROW_HEIGHT_PX = 30`, `TM_HIST_LEN = 100`, `TM_HIST_SHORT = 30`, `TM_NAME_MAX = 256`

### Prompt 1.3 — Design Pattern Opportunities

> "For this C GUI application (a Task Manager using raylib), identify which classic design patterns would improve the architecture. For each pattern, show: (1) the concrete problem in the current code that the pattern solves, (2) the C implementation using structs and function pointers, (3) a before/after code snippet using actual names from the codebase. Cover at minimum: Strategy pattern for tab rendering dispatch (the current if/else chain in main loop), Observer pattern for UI refresh after process list changes, Command pattern for button click handlers, and a Platform Adapter pattern for abstracting the #ifdef _WIN32 blocks."

**Purpose:** We knew the code had structural problems but needed to know which patterns to apply in C — not C++ or Java. This prompt produced concrete function-pointer-based implementations that directly shaped the ver2 architecture.

---

## Stage 2: Coding Convention & Standards Prompts

### Prompt 2.1 — Naming Convention Design

> "Create a comprehensive naming convention document for the C Task Manager project. The project uses C99/C11. Address each element: (1) Struct and typedef names — the current code has 'Process', 'StartupApp', 'AppHistory', 'PerformanceData', 'Button', 'Tab', 'ScrollBar'; (2) Enum names and values — current: 'TabType', 'TAB_PROCESSES', 'TAB_COUNT'; (3) Function names — current examples: 'DrawProcessTab', 'GetProcessList', 'HandleScrollBarInteraction', 'UpdatePerformanceData', 'ToggleStartupApp'; (4) Global and module-level variable names — current: 'processList', 'selectedProcessIndex', 'processScrollBar', 'cpuCoreUsage'; (5) Constants and macros — current: 'BG_COLOR', 'HEADER_COLOR', 'RESIZE_BORDER'. For each category produce a before/after table and state the rule in one sentence."

**Purpose:** Inconsistent naming was the most visible readability problem in ver1. `DrawProcessTab` uses PascalCase, `processList` uses camelCase, `BG_COLOR` uses SCREAMING_SNAKE_CASE, and `PROCESS_CMD` is a macro disguised as a constant. We needed one rule per category that everyone on the team would apply consistently.

**Convention decisions made:**

| Element | Rule | Before (ver1) | After (ver2) |
|---------|------|--------------|-------------|
| Struct types | `TmPascalCase` prefix | `Process` | `TmProcess` |
| Struct fields | `snake_case` + unit suffix | `isSelected` | `is_selected` |
| Struct fields | `snake_case` + unit suffix | `memory` (ambiguous units) | `memory_bytes` |
| Enum type | `TmPascalCase` | `TabType` | `TmTabId` |
| Enum values | `TM_UPPER_SNAKE` | `TAB_PROCESSES` | `TM_TAB_PROCESSES` |
| Public functions | `tm_noun_verb()` | `GetProcessList()` | `tm_process_list_refresh()` |
| Public functions | `tm_noun_verb()` | `DrawProcessTab()` | `ui_tab_process_draw()` |
| Public functions | `tm_noun_verb()` | `HandleScrollBarInteraction()` | `ui_scrollbar_handle()` |
| Static helpers | `local_snake_case()` | `UpdateScrollBars()` | `update_scrollbar_content()` |
| Color constants | `TM_COLOR_NAME` | `BG_COLOR` | `TM_COLOR_BG` |
| Size constants | `TM_NOUN_PX` / `TM_NOUN_LEN` | magic `30` | `TM_ROW_HEIGHT_PX` |
| Integer types | `stdint.h` explicit types | `unsigned long` PID | `uint32_t pid` |
| Integer types | `stdint.h` explicit types | `unsigned long` memory | `uint64_t memory_bytes` |
| App state | Single `TmAppState` struct | 15 bare globals | `TmAppState` passed by pointer |

### Prompt 2.2 — Magic Number Elimination

> "Scan this C source code and list every magic number (unnamed numeric literal) that appears in function bodies. For each one, give the value, the context where it is used, and the named constant it should become. Organize results into these categories: UI layout constants, buffer sizes, timing constants, and data structure sizes. Then show what the tm_types.h header section for all these constants should look like, using the TM_ prefix convention."

**Purpose:** Magic numbers were the second most visible problem. The number `30` appeared with three different meanings in the same file: row height in pixels, history ring buffer size, and a scroll step in wheel events. Without named constants, a reader has no way to know which `30` is which.

**All magic numbers found and replaced:**

| Value | Context in ver1 | Named constant in ver2 |
|-------|-----------------|------------------------|
| `30` | Row height in pixels | `TM_ROW_HEIGHT_PX` |
| `45` | Startup row height | `TM_STARTUP_ROW_PX` |
| `65` | App history row height | `TM_HISTORY_ROW_PX` |
| `30` | History ring buffer length | `TM_HIST_SHORT` |
| `100` | Performance history length | `TM_HIST_LEN` |
| `256` | Name/publisher buffer size | `TM_NAME_MAX` |
| `32` | Status string buffer | `TM_STATUS_MAX` |
| `50` | Button text buffer | (folded into `TmButton.text[50]`) |
| `8` | Startup app count | `TM_MAX_STARTUP_APPS` |
| `8` | History app count | `TM_MAX_HISTORY_APPS` |
| `8` | CPU core count | `TM_CORE_COUNT` |
| `8` | Resize border px | `TM_RESIZE_BORDER_PX` |
| `1200` | Default window width | `TM_DEFAULT_W` |
| `800` | Default window height | `TM_DEFAULT_H` |
| `800` | Minimum window width | `TM_MIN_WINDOW_W` |
| `600` | Minimum window height | `TM_MIN_WINDOW_H` |
| `60` | Target FPS | `TM_TARGET_FPS` |
| `120` | Message display frames | `TM_MSG_DISPLAY_FRAMES` |
| `60` | Short message frames | `TM_MSG_SHORT_FRAMES` |
| `1.0f` | Performance update interval | `TM_PERF_UPDATE_INTERVAL_S` |
| `2.0f` | History update interval | `TM_HISTORY_UPDATE_INTERVAL_S` |

### Prompt 2.3 — Documentation Standard

> "Create a Doxygen-style documentation standard for the C Task Manager project. Provide complete templates for: (1) file header comment (with @file, @brief, @note, @version), (2) public function comment (with @brief, @param, @return, @note), (3) struct definition comment, (4) inline comment guidelines emphasizing 'explain why, not what'. Then generate complete Doxygen headers for these specific functions using the actual code: UpdatePerformanceData(), HandleScrollBarInteraction(), KillProcess(), GetProcessList(). Include a note about when NOT to comment (trivial assignments, obvious loop variables)."

**Purpose:** Ver1 had zero comments — not a single `//` line in 800 lines of code. Any new team member or the author returning after three months would have no guidance on what `cpuHistoryIndex` tracks, why `thumbHeight` has a 30-pixel minimum, or what `pclose(fp)` is releasing.

---

## Stage 3: Folder Structure Reorganization Prompts

### Prompt 3.1 — Module Architecture Design

> "The current C Task Manager is a single 800-line file. Propose a multi-file, multi-directory layout that separates: (1) data structure management (linked lists for Process, StartupApp, AppHistory), (2) performance data simulation, (3) platform-specific OS calls (Windows tasklist vs Linux ps), (4) UI rendering (one file per tab, plus shared scrollbar and button modules), (5) application-wide types and constants (a single tm_types.h), (6) logging/utilities. Show the complete directory tree with a one-line description for each file. The design must ensure that platform files have zero raylib calls, core files have zero #ifdef blocks, and ui files have zero popen() calls."

**Purpose:** The single-file structure made it impossible to unit-test data logic without starting a raylib window, impossible to compile the Windows process parser on Linux, and impossible for two developers to work on different tabs without merge conflicts. A clear folder structure is the prerequisite for all subsequent module work.

**Resulting directory structure (implemented in ver2):**

```
task_manager/
├── src/
│   ├── main.c                       # Entry point (~30 executable lines)
│   ├── core/
│   │   ├── tm_process.c             # Process linked list, observer registry
│   │   ├── tm_startup.c             # Startup app list management
│   │   ├── tm_app_history.c         # Per-app history ring buffers
│   │   └── tm_perf.c                # Performance data update orchestration
│   ├── platform/
│   │   ├── platform_win32.c         # Windows: tasklist CSV parsing, SIGKILL
│   │   └── platform_posix.c         # Linux/macOS: ps parsing, kill()
│   ├── ui/
│   │   ├── ui_core.c                # Init, layout, input dispatch, draw orchestration
│   │   ├── ui_scrollbar.c           # Scrollbar hit-testing and rendering
│   │   ├── ui_button.c              # Button rendering
│   │   ├── ui_theme.c               # Color constants as TM_COLOR_* defines
│   │   ├── ui_tab_processes.c       # Processes tab rendering
│   │   ├── ui_tab_performance.c     # Performance tab rendering
│   │   ├── ui_tab_history.c         # App History tab rendering
│   │   └── ui_tab_startup.c         # Startup tab rendering
│   └── utils/
│       └── tm_log.c                 # Timestamped logger (DEBUG/INFO/WARN/ERROR)
├── include/
│   ├── tm_types.h                   # ALL structs, enums, constants, TmAppState
│   ├── tm_platform.h                # TmPlatform vtable, g_platform extern
│   ├── tm_process.h                 # tm_process_* public API
│   ├── tm_startup.h                 # tm_startup_* public API
│   ├── tm_app_history.h             # tm_history_* public API
│   ├── tm_perf.h                    # tm_perf_* public API
│   ├── tm_ui.h                      # ui_* public API
│   └── tm_log.h                     # tm_log_* macros and function
├── CMakeLists.txt                   # Build system
└── README.md
```

### Prompt 3.2 — Header File Design

> "Design the header files for the C Task Manager ver2 modular structure. For tm_types.h, show the complete file: include guards, all #define constants (replacing magic numbers), the tm_result_t enum, TmTabId enum, all struct definitions (TmProcess, TmStartupApp, TmAppHistory, TmPerfData, TmButton, TmTab, TmScrollBar), the TmAppState 'god state' struct that replaces all global variables, the TmObserver struct, and the TmTabDescriptor struct for the Strategy pattern. For tm_platform.h, show the TmPlatform vtable struct with all five function pointers. Explain why ALL other headers should include only tm_types.h and their own declarations — never each other."

**Purpose:** Without proper header design, circular includes, missing declarations, and implicit function assumptions would cause hard-to-diagnose compilation errors. We needed one canonical dependency rule: everything depends on `tm_types.h`; nothing else cross-includes.

---

## Stage 4: Code-Level Improvement Prompts

### Prompt 4.1 — Global State Elimination

> "The ver1 Task Manager has 15 module-level static globals: processList, startupList, appHistoryList, processCount, selectedProcessIndex, selectedStartupIndex, tabs[], refreshBtn, endTaskBtn, enableStartupBtn, disableStartupBtn, message[], messageTimer, messageColor, perfData, cpuCoreUsage[], screenWidth, screenHeight, processScrollBar, startupScrollBar, appHistoryScrollBar, isResizing. Design a TmAppState struct that holds all of these. Show the struct definition in full. Then show how three representative functions change their signatures: GetProcessList(), DrawProcessTab(), and HandleScrollBarInteraction(). Explain how passing TmAppState* makes hidden dependencies visible and how passing const TmAppState* enforces read-only access in drawing functions."

**Purpose:** Global state is the root cause of many bugs in the ver1 code. `DrawProcessTab()` silently reads `processScrollBar`, `screenWidth`, `processCount`, `processList`, and `perfData` — none of which appear in its signature. This makes the function impossible to test in isolation and impossible to reason about without reading the entire file.

**Before (ver1) — hidden global dependencies:**
```c
// ver1: what does this function actually depend on? You must read the body.
static void DrawProcessTab(void) {
    int startY = 120;
    int contentWidth = screenWidth - 30;           // reads global
    int processListHeight = screenHeight - 200;    // reads global
    UpdateScrollBars();                            // mutates globals
    HandleScrollBarInteraction(&processScrollBar, startY, processListHeight);
    // ... draws processList (global), perfData (global)
}
```

**After (ver2) — explicit dependencies:**
```c
// ver2: all dependencies visible in the signature
void ui_tab_process_draw(const TmAppState *s) {
    int start_y      = 120;
    int content_w    = s->screen_w - 30;
    int list_height  = s->screen_h - 200;
    // scrollbar is s->process_scroll — compiler enforces read-only via const
}
```

### Prompt 4.2 — Function Decomposition

> "Break down these three functions from the ver1 Task Manager into smaller, focused helpers. For each, show the original function, list the sub-functions it should be split into with their signatures, and show the new orchestrator that calls them. Functions: (1) DrawPerformanceTab() — 95 lines drawing CPU graph, memory bar, GPU graph, disk bar, and system info; (2) InitializeApp() — 65 lines initializing tabs, buttons, perfData, scrollbars, startup list, and app history demo data; (3) UpdatePerformanceData() — 60 lines updating CPU, memory, disk, GPU metrics plus calling UpdateAppHistory(). Each new function must have exactly one job."

**Purpose:** Functions longer than 40-50 lines in a UI application almost always have multiple responsibilities. When a real OS CPU API is ready to replace the simulated `rand()` call, we should only need to touch one function — not a 95-line monolith that also draws the GPU graph.

**Key decompositions performed:**

| Original Function (ver1) | Split Into (ver2) | Location |
|--------------------------|-------------------|----------|
| `DrawPerformanceTab()` (95 lines) | `ui_tab_perf_draw_cpu()`, `ui_tab_perf_draw_memory()`, `ui_tab_perf_draw_gpu()`, `ui_tab_perf_draw_disk()`, `ui_tab_perf_draw_sysinfo()` | `ui_tab_performance.c` |
| `InitializeApp()` (65 lines) | `init_tabs()`, `init_buttons()`, `layout_scrollbars()`, `tm_perf_data_init()`, `tm_history_init()`, `tm_startup_list_load()` | `ui_core.c`, `tm_perf.c`, `tm_startup.c`, `tm_app_history.c` |
| `UpdatePerformanceData()` (60 lines) | `tm_perf_update_cpu()`, `tm_perf_update_memory()`, `tm_perf_update_disk()`, `tm_perf_update_gpu()`, `tm_history_update()` | `tm_perf.c`, `tm_app_history.c` |
| `GetProcessList()` (40 lines) | `tm_process_list_refresh()`, `prepend_process()`, `notify_observers()`, platform: `win32_open_process_list()`, `win32_parse_process_line()`, `posix_open_process_list()`, `posix_parse_process_line()` | `tm_process.c`, `platform_win32.c`, `platform_posix.c` |
| `DrawProcessTab()` (55 lines) | `ui_tab_process_draw()` calls `ui_scrollbar_handle()` separately; header drawn inline; rows drawn in loop | `ui_tab_processes.c`, `ui_scrollbar.c` |
| `UpdateButtons()` (70 lines) | `handle_tab_clicks()`, `handle_keyboard()`, `handle_process_selection()`, `handle_startup_selection()`, `handle_scrollbars()` + Command functions: `cmd_refresh()`, `cmd_end_task()`, `cmd_enable_startup()`, `cmd_disable_startup()` | `ui_core.c` |

### Prompt 4.3 — Error Handling Improvement

> "Improve error handling throughout the C Task Manager. Current problems: (1) GetProcessList() calls popen() and on failure sets a message string — but the caller never knows an error occurred; (2) malloc() failure in GetProcessList(), GetStartupApps(), and InitializeApp() is handled with 'if (newProc == NULL) continue' — allocation failure is silently swallowed; (3) KillProcess() returns bool but can't distinguish 'process not found' from 'permission denied'. Design a tm_result_t enum with at least 5 error codes. Show how GetProcessList() is rewritten to return tm_result_t and propagate errors upward. Show the TM_CHECK() macro for eliminating boilerplate error propagation."

**Purpose:** Silent error swallowing makes bugs nearly impossible to diagnose. When `malloc` fails and `continue` skips the entry, the process list silently shows fewer processes with no indication that anything went wrong. A typed result enum gives callers enough information to log, retry, or show a meaningful error message.

**Error codes defined in ver2:**
```c
typedef enum {
    TM_OK              =  0,
    TM_ERR_ALLOC       = -1,  /* malloc / arena exhausted     */
    TM_ERR_IO          = -2,  /* popen / file failure         */
    TM_ERR_PLATFORM    = -3,  /* OS call (kill, etc.) failed  */
    TM_ERR_INVALID_ARG = -4,  /* NULL or out-of-range param   */
} tm_result_t;
```

**TM_CHECK macro eliminates boilerplate:**
```c
/* Every public function call in a chain: */
static void app_init(TmAppState *s) {
    tm_perf_data_init(&s->perf);
    ui_init(s);
    if (tm_startup_list_load(s) != TM_OK)
        tm_log_warn("Startup list load failed");
    if (tm_history_init(s) != TM_OK)
        tm_log_warn("History init failed");
}
```

### Prompt 4.4 — Platform Abstraction via Vtable

> "Refactor the platform-specific code in the C Task Manager. Currently the #ifdef _WIN32 blocks appear inside GetProcessList() (line-parsing logic), KillProcess() (command format), and the #define macros PROCESS_CMD and KILL_CMD at the top. Design a TmPlatform struct with five function pointers: open_process_list(), parse_process_line(), kill_process(), sample_cpu(), and query_memory(). Show the complete struct definition, how main.c selects the adapter in one #ifdef block, and how tm_process_list_refresh() becomes completely #ifdef-free. Show the complete platform_win32.c and platform_posix.c adapter implementations."

**Purpose:** Scattered `#ifdef` blocks make the code look like a patchwork of platform hacks. The platform vtable achieves the same result — Windows uses `tasklist`, Linux uses `ps` — but confines ALL platform knowledge to two files. Every other file in the project becomes fully portable.

---

## Stage 5: Documentation & Guide Prompts

### Prompt 5.1 — Refactoring Guide Creation

> "Generate a comprehensive REFACTORING_GUIDE.md for the C Task Manager project. The document should cover: (1) executive summary of current state vs. desired state; (2) naming convention table with before/after examples drawn directly from the source (Process → TmProcess, GetProcessList → tm_process_list_refresh, BG_COLOR → TM_COLOR_BG, etc.); (3) target folder structure with rationale for each directory; (4) programming style guidelines for indentation, function length, error handling, and memory ownership; (5) full C implementations of three design patterns — Strategy (tab renderer table), Observer (process change notification), Command (button actions); (6) an eight-phase migration roadmap that leaves the application compiling and runnable at the end of every phase."

**Purpose:** A living guide document ensures that refactoring decisions are not lost after the project is submitted. Anyone continuing the project — or the same developers returning in six months — can read the guide to understand why every naming and structural decision was made.

### Prompt 5.2 — Before/After Comparison Document

> "Generate a detailed before/after code comparison document for the C Task Manager refactoring. For each major change, show the exact ver1 code and the exact ver2 code side-by-side in fenced code blocks. Cover: (1) naming: one function rename and one struct rename with actual code; (2) global state: a representative function with hidden globals vs. explicit TmAppState*; (3) error handling: malloc failure handling and popen failure handling; (4) function decomposition: InitializeApp() before (65-line monolith) vs. after (orchestrator + sub-functions); (5) platform isolation: the #ifdef block inside GetProcessList() vs. the vtable dispatch in tm_process_list_refresh(); (6) design patterns: the if/else tab dispatch chain vs. the TmTabDescriptor strategy table."

**Purpose:** Showing the same code before and after is more persuasive than describing the change in prose. The comparison document became the primary reference for code review and also served as evidence of the breadth of changes made during refactoring.

---

## Prompt Design Philosophy

The most important decision in prompt design for this project was to always reference the actual code by name. Generic prompts like "find code smells in this C file" produce generic results like "consider adding comments" or "some functions are long." Prompts that name specific artifacts — `DrawPerformanceTab`, `processScrollBar`, `HandleScrollBarInteraction`, `TAB_COUNT` — force the AI to engage with the specific code rather than generating boilerplate advice. This produced output we could use directly rather than output we had to mentally translate into our codebase.

The second key decision was the staged approach. Each stage had a clearly defined output artifact: Stage 1 produced an inventory and violation list, Stage 2 produced naming tables and constant lists, Stage 3 produced a directory tree and header skeletons, Stage 4 produced refactored function implementations, Stage 5 produced documentation. Each stage's output became the input for the next prompt. This prevented the "do everything at once" failure mode where a single large prompt produces shallow coverage of many changes rather than deep coverage of one.

The third decision was to separate analysis from implementation. We never asked AI to "analyze and then refactor" in a single prompt. Analysis prompts asked only for findings and recommendations. Implementation prompts referenced the findings document and asked for concrete code. This separation allowed us to review the plan before any code was generated and to reject or modify recommendations before they were built into the implementation.

Finally, when AI suggestions needed adjustment — for example, when AI suggested using C++ templates or `std::vector` — we used correction prompts like "this must be pure C99, no C++ constructs" rather than abandoning the output. About 20% of AI-generated code required modification, mostly to remove platform assumptions or to match the naming convention exactly.

---

## Summary of Prompt Categories

| Category | # Prompts | Primary Output Produced |
|----------|-----------|-------------------------|
| Analysis & Planning | 3 | Function/global/struct inventory, refactoring opportunity list, design pattern recommendations |
| Naming Conventions | 3 | Naming rule table (14 categories), magic number replacement list (21 constants), Doxygen templates |
| Folder Structure | 2 | 26-file directory tree, complete `tm_types.h` design, `tm_platform.h` vtable design |
| Code-Level Changes | 4 | `TmAppState` struct, function decomposition plan, `tm_result_t` error system, platform vtable implementations |
| Documentation | 2 | `REFACTORING_GUIDE.md` (8-phase roadmap), Before/After Comparison document |
| **Total** | **14** | **Complete ver1 → ver2 transformation** |

---

# PART B — DETAILED EXPLANATION OF CHANGES

---

## 9. Executive Summary

The C Task Manager is a cross-platform system monitor that replicates the core features of the Windows Task Manager. It displays four tabs: Processes (live list from `popen`), Performance (animated CPU/GPU/Memory/Disk graphs), App History (per-application resource rings), and Startup (list with enable/disable toggle). The application is built with raylib and targets both Windows and Linux from a single codebase.

Version 1 was an 800-line single-file C program. All data structures, platform-specific OS calls, UI rendering logic, scroll bar mathematics, button state management, and application initialization lived in one translation unit with no header files (other than `raylib.h`), 15 bare global variables, no typed error system, and `#ifdef _WIN32` blocks scattered throughout. It was functionally complete but impossible to extend without reading the entire file, and impossible to test without running a window.

Version 2 is a 26-file, 6-directory modular project totalling approximately 2,400 lines of code and documentation. The single `#ifdef _WIN32` block lives in `main.c`; everything else is platform-neutral. All mutable state lives in a single `TmAppState` struct passed by pointer. Every fallible function returns a `tm_result_t`. Three design patterns — Strategy, Observer, Command — are explicitly implemented with function pointers. All 21 magic numbers have named constants in `tm_types.h`. Doxygen-style comments appear on every public function and struct.

The three most impactful improvements were: (1) the `TmAppState` struct that made all function dependencies explicit and compiler-enforced; (2) the `TmPlatform` vtable that eliminated all `#ifdef` blocks outside the platform directory; and (3) the `tm_result_t` error system that replaced silent `continue` on allocation failure with logged, propagated errors.

What still needs to be done: real OS API integration (currently CPU/memory/disk use `rand()`), unit tests for the core and platform modules, and completion of the Phase 4–8 items in the refactoring roadmap.

---

## 10. Coding & Naming Conventions

Ver1 had no explicit naming convention. Functions used PascalCase (`DrawProcessTab`, `GetProcessList`), local variables used camelCase (`selectedProcessIndex`, `cpuCoreUsage`), global color constants used SCREAMING_SNAKE_CASE (`BG_COLOR`, `HEADER_COLOR`), and struct fields mixed camelCase (`isSelected`, `isHovered`) with no unit suffixes (`memory` could be bytes, KB, or MB — it was actually bytes/1024 depending on context). Two struct types had inconsistent capitalization: `Process` vs `ScrollBar` vs `PerformanceData`.

Ver2 applies one rule per element category, enforced by the naming convention document generated in Stage 2:

**Complete before/after naming table:**

| Element Type | ver1 Name | ver2 Name | Rule Applied |
|-------------|-----------|-----------|-------------|
| Struct: process entry | `Process` | `TmProcess` | `Tm` prefix + PascalCase |
| Struct: startup app | `StartupApp` | `TmStartupApp` | `Tm` prefix + PascalCase |
| Struct: history entry | `AppHistory` | `TmAppHistory` | `Tm` prefix + PascalCase |
| Struct: performance | `PerformanceData` | `TmPerfData` | `Tm` prefix + PascalCase |
| Struct: button | `Button` | `TmButton` | `Tm` prefix + PascalCase |
| Struct: tab | `Tab` | `TmTab` | `Tm` prefix + PascalCase |
| Struct: scrollbar | `ScrollBar` | `TmScrollBar` | `Tm` prefix + PascalCase |
| Struct field: selected | `isSelected` | `is_selected` | snake_case fields |
| Struct field: hovered | `isHovered` | `is_hovered` | snake_case fields |
| Struct field: pid | `unsigned long pid` | `uint32_t pid` | explicit integer types |
| Struct field: memory | `unsigned long memory` | `uint64_t memory_bytes` | unit suffix |
| Struct field: impact | `float impact` | `float impact_s` | unit suffix (seconds) |
| Struct field: scroll pos | `scrollPosition` | `scroll_pos` | snake_case |
| Enum type | `TabType` | `TmTabId` | `Tm` prefix + PascalCase |
| Enum value | `TAB_PROCESSES` | `TM_TAB_PROCESSES` | `TM_` prefix |
| Enum value | `TAB_COUNT` | `TM_TAB_COUNT` | `TM_` prefix |
| Function: refresh list | `GetProcessList()` | `tm_process_list_refresh()` | `module_noun_verb` |
| Function: free list | `CleanupProcessList()` | `tm_process_list_free()` | `module_noun_verb` |
| Function: draw tab | `DrawProcessTab()` | `ui_tab_process_draw()` | `module_noun_verb` |
| Function: scrollbar | `HandleScrollBarInteraction()` | `ui_scrollbar_handle()` | `module_noun_verb` |
| Function: kill process | `KillProcess()` | `tm_process_kill()` | `module_noun_verb` |
| Function: toggle app | `ToggleStartupApp()` | `tm_startup_toggle()` | `module_noun_verb` |
| Color constant | `BG_COLOR` | `TM_COLOR_BG` | `TM_COLOR_` prefix |
| Size constant | magic `30` | `TM_ROW_HEIGHT_PX` | `TM_NOUN_UNIT` |
| Size constant | magic `100` | `TM_HIST_LEN` | `TM_NOUN_LEN` |

**Rationale for `tm_noun_verb` function naming:** When functions are sorted in an IDE or `grep` output, `tm_process_*` groups all process functions together, `tm_startup_*` groups startup functions, and `ui_tab_*` groups all tab renderers. In ver1, `GetProcessList`, `CleanupProcessList`, `DrawProcessTab`, and `HandleSelection` are four functions that all relate to the process list — but their names give no indication of this relationship.

---

## 11. Design Model

Ver2 is organized into four layers with a strict top-down dependency rule: each layer may only call downward; no upward calls are permitted.

```
┌─────────────────────────────────────────────────────┐
│  ENTRY POINT                                         │
│  main.c — selects platform, calls app_init/update/  │
│  draw/cleanup. ~30 executable lines.                 │
└──────────────────────┬──────────────────────────────┘
                       │ calls
┌──────────────────────▼──────────────────────────────┐
│  UI LAYER  (src/ui/)                                 │
│  ui_core.c, ui_scrollbar.c, ui_button.c,             │
│  ui_tab_processes.c, ui_tab_performance.c,           │
│  ui_tab_history.c, ui_tab_startup.c, ui_theme.c      │
│  — All raylib calls live here. Zero popen() calls.   │
└──────────────────────┬──────────────────────────────┘
                       │ calls
┌──────────────────────▼──────────────────────────────┐
│  CORE LAYER  (src/core/)                             │
│  tm_process.c, tm_startup.c, tm_app_history.c,       │
│  tm_perf.c                                           │
│  — Linked list management, ring buffer updates,      │
│    Observer notifications. Zero raylib calls.        │
└──────────────────────┬──────────────────────────────┘
                       │ calls
┌──────────────────────▼──────────────────────────────┐
│  PLATFORM LAYER  (src/platform/)                     │
│  platform_win32.c, platform_posix.c                  │
│  — OS process enumeration, kill(), memory queries.   │
│    All #ifdef _WIN32 lives here. Zero raylib calls.  │
└──────────────────────┬──────────────────────────────┘
                       │ calls
┌──────────────────────▼──────────────────────────────┐
│  SYSTEM HEADERS                                      │
│  <stdio.h>, <stdlib.h>, <signal.h>, <unistd.h>       │
│  Windows.h (only in platform_win32.c)               │
└─────────────────────────────────────────────────────┘
```

**Layer access rules (what each layer is NOT allowed to do):**

- **Platform Layer** must not call any raylib function (`DrawRectangle`, `GetMousePosition`, etc.) — it works only with `FILE*`, integer PIDs, and `tm_result_t` return codes
- **Core Layer** must not call `popen()`, `system()`, or any OS-specific function directly — it calls through `g_platform->*` function pointers only
- **UI Layer** must not call `popen()`, `system()`, or `g_platform->*` directly — it reads data from `TmAppState` and calls core functions for mutations
- **Entry Point** (main.c) contains the single permitted `#ifdef _WIN32` block to select the platform adapter

---

## 12. Design Patterns Analysis

### 12.1 Strategy Pattern — Tab Renderer Dispatch

**Problem in ver1:** The main draw loop used an `if/else` chain to select the tab renderer. Adding a fifth tab required modifying the main loop.

```c
/* ver1 — if/else chain, must be modified for every new tab */
if (tabs[TAB_PROCESSES].isActive) {
    DrawProcessTab();
} else if (tabs[TAB_PERFORMANCE].isActive) {
    DrawPerformanceTab();
} else if (tabs[TAB_APP_HISTORY].isActive) {
    DrawAppHistoryTab();
} else if (tabs[TAB_STARTUP].isActive) {
    DrawStartupTab();
}
```

**ver2 solution — function-pointer descriptor table:**
```c
/* ver2 — table-driven dispatch: adding a tab = adding one row */
static const TmTabDescriptor k_tabs[TM_TAB_COUNT] = {
    { TM_TAB_PROCESSES,   "Processes",   ui_tab_process_draw  },
    { TM_TAB_PERFORMANCE, "Performance", ui_tab_perf_draw     },
    { TM_TAB_APP_HISTORY, "App History", ui_tab_history_draw  },
    { TM_TAB_STARTUP,     "Startup",     ui_tab_startup_draw  },
};

/* Dispatch — unchanged regardless of how many tabs exist */
void ui_content_draw(const TmAppState *s) {
    for (int i = 0; i < TM_TAB_COUNT; i++) {
        if (k_tabs[i].id == s->active_tab) {
            k_tabs[i].draw(s);
            return;
        }
    }
}
```

### 12.2 Command Pattern — Button Actions

**Problem in ver1:** `UpdateButtons()` was a 70-line function that detected mouse clicks AND executed business logic (killing processes, refreshing the list, toggling startup apps) all inline. UI event detection was coupled with application logic.

**ver2 solution — each action is a named `TmCommandFn` function:**
```c
/* ver2 — each button action is a separate function */
static tm_result_t cmd_refresh(TmAppState *s, void *param) {
    (void)param;
    TM_CHECK(tm_process_list_refresh(s));
    TM_CHECK(tm_perf_update(s));
    ui_toast_show(s, "Refreshed", GREEN, TM_MSG_SHORT_FRAMES);
    return TM_OK;
}

static tm_result_t cmd_end_task(TmAppState *s, void *param) {
    (void)param;
    TmProcess *sel = tm_process_get_selected(s);
    if (!sel) return TM_ERR_INVALID_ARG;
    tm_result_t r = tm_process_kill(sel->pid);
    if (r == TM_OK) {
        ui_toast_show(s, "Process terminated", GREEN, TM_MSG_DISPLAY_FRAMES);
        tm_process_list_refresh(s);
    } else {
        ui_toast_show(s, "Failed to terminate process", RED, TM_MSG_DISPLAY_FRAMES);
    }
    return r;
}
/* cmd_enable_startup, cmd_disable_startup follow the same pattern */
```

Benefits: each command is independently testable, independently nameable in logs, and independently replaceable. Undo support becomes possible by storing command history.

### 12.3 Observer Pattern — Process Change Notification

**Problem in ver1:** `GetProcessList()` directly mutated UI globals (`endTaskBtn.enabled = false`, `selectedProcessIndex = -1`) from inside the core data function. The core was coupled to the UI.

**ver2 solution — core fires callbacks; UI registers what it needs:**
```c
/* ver2 — observer registered in main.c */
static void on_process_changed(const TmAppState *s, void *user) {
    (void)user;
    ui_layout_update((TmAppState *)s);   /* UI responds to data change */
}

/* In app_init(): */
tm_process_observer_add(on_process_changed, NULL);

/* In tm_process.c — core fires callback, knows nothing about UI */
static void notify_observers(const TmAppState *s) {
    for (int i = 0; i < s_observer_count; i++) {
        if (s_observers[i].on_process_changed)
            s_observers[i].on_process_changed(s, s_observers[i].user_data);
    }
}
```

### 12.4 Platform Adapter Pattern — OS Abstraction Vtable

**Problem in ver1:** `#ifdef _WIN32` appeared inside `GetProcessList()` (line parsing), `KillProcess()` (command format), and at the top of the file (`#define PROCESS_CMD`). Every new platform required modifying these functions.

**ver2 solution — one vtable struct, two implementations, one selection in main.c:**
```c
/* tm_platform.h — the only OS abstraction interface */
typedef struct {
    FILE        *(*open_process_list)(void);
    bool         (*parse_process_line)(FILE *fp, TmProcess *out);
    tm_result_t  (*kill_process)(uint32_t pid);
    float        (*sample_cpu)(void);
    void         (*query_memory)(uint64_t *used_kb, uint64_t *total_kb);
} TmPlatform;

/* main.c — the ONLY #ifdef _WIN32 in the project */
#ifdef _WIN32
    g_platform = &k_platform_win32;
#else
    g_platform = &k_platform_posix;
#endif

/* tm_process.c — zero #ifdef, fully portable */
tm_result_t tm_process_list_refresh(TmAppState *s) {
    FILE *fp = g_platform->open_process_list();
    if (!fp) return TM_ERR_IO;
    TmProcess entry = {0};
    while (g_platform->parse_process_line(fp, &entry))
        prepend_process(s, &entry);
    pclose(fp);
    notify_observers(s);
    return TM_OK;
}
```

### 12.5 Linked List as Consistent Data Pattern

All three data collections — process list (`TmProcess*`), startup list (`TmStartupApp*`), app history (`TmAppHistory*`) — use the same intrusive singly-linked list pattern. Each node owns its own next pointer; each module owns its free function (`tm_process_list_free`, `tm_startup_list_free`, `tm_history_list_free`). This consistency means any developer who understands one list understands all three.

---

## 13. Detailed Change Log (ver1 → ver2)

### 13.1 Structural Changes

| Component | ver1 | ver2 | Change |
|-----------|------|------|--------|
| .c source files | 1 | 16 | +15 |
| .h header files | 0 | 8 | +8 |
| Directories | 1 (flat) | 6 (layered) | +5 |
| Total lines (code + docs) | ~800 | ~2,400 | +1,600 |
| Module-level globals | 15 bare statics | 0 (all in TmAppState) | -15 |
| `#ifdef _WIN32` locations | 3 (scattered) | 1 (main.c only) | -2 |
| Error handling returns | `bool` or `void` | `tm_result_t` on all fallible fns | systematic |
| Build system | None (manual gcc) | CMakeLists.txt | added |

### 13.2 Function Changes

| ver1 Function | ver2 Equivalent(s) | Module |
|--------------|-------------------|--------|
| `UpdateUIElements()` | `ui_layout_update()` + sub-functions | `ui_core.c` |
| `InitializeApp()` | `app_init()` → `ui_init()`, `tm_perf_data_init()`, `tm_history_init()`, `tm_startup_list_load()` | `main.c`, split across modules |
| `CleanupProcessList()` | `tm_process_list_free()` | `tm_process.c` |
| `CleanupStartupList()` | `tm_startup_list_free()` | `tm_startup.c` |
| `CleanupAppHistory()` | `tm_history_list_free()` | `tm_app_history.c` |
| `GetProcessList()` | `tm_process_list_refresh()` + platform `open_process_list()` + `parse_process_line()` | `tm_process.c`, platform files |
| `GetStartupApps()` | `tm_startup_list_load()` | `tm_startup.c` |
| `UpdateAppHistory()` | `tm_history_update()` | `tm_app_history.c` |
| `UpdatePerformanceData()` | `tm_perf_update()` → per-metric helpers | `tm_perf.c` |
| `UpdateScrollBars()` | `update_scrollbar_content()` | `ui_core.c` (static) |
| `HandleScrollBarInteraction()` | `ui_scrollbar_handle()` | `ui_scrollbar.c` |
| `DrawProcessTab()` | `ui_tab_process_draw()` | `ui_tab_processes.c` |
| `DrawPerformanceTab()` | `ui_tab_perf_draw()` + 5 section helpers | `ui_tab_performance.c` |
| `DrawAppHistoryTab()` | `ui_tab_history_draw()` | `ui_tab_history.c` |
| `DrawStartupTab()` | `ui_tab_startup_draw()` | `ui_tab_startup.c` |
| `DrawTabs()` | `ui_tabs_draw()` | `ui_core.c` |
| `DrawButtons()` | `ui_buttons_draw()` | `ui_core.c` |
| `KillProcess()` | `tm_process_kill()` → `g_platform->kill_process()` | `tm_process.c`, platform files |
| `ToggleStartupApp()` | `tm_startup_toggle()` | `tm_startup.c` |
| `UpdateButtons()` | `ui_input_update()` + command functions | `ui_core.c` |
| `HandleSelection()` | `handle_process_selection()`, `handle_startup_selection()` | `ui_core.c` (static) |
| `DrawMessage()` | `ui_toast_draw()` | `ui_core.c` |
| `HandleInput()` | `handle_keyboard()` | `ui_core.c` (static) |
| `HandleWindowResize()` | `ui_window_resize_handle()` | `ui_core.c` |
| `DrawResizeHandle()` | `ui_resize_handle_draw()` | `ui_core.c` |
| `IsMouseOverResizeHandle()` | inlined into `ui_window_resize_handle()` | `ui_core.c` |
| *(new)* | `tm_log()`, `tm_log_info/warn/error` macros | `tm_log.c` |
| *(new)* | `tm_process_observer_add()`, `notify_observers()` | `tm_process.c` |
| *(new)* | `tm_process_get_selected()` | `tm_process.c` |
| *(new)* | `ui_toast_show()`, `ui_toast_tick()` | `ui_core.c` |

### 13.3 Struct Changes

| Struct | ver1 | ver2 | Changes |
|--------|------|------|---------|
| Process entry | `Process` | `TmProcess` | Renamed; `unsigned long pid` → `uint32_t pid`; `unsigned long memory` → `uint64_t memory_bytes`; `isSelected` → `is_selected`; field names to snake_case |
| Startup app | `StartupApp` | `TmStartupApp` | Renamed; `bool enabled` → `bool is_enabled`; `float impact` → `float impact_s` (unit suffix); fields to snake_case |
| App history | `AppHistory` | `TmAppHistory` | Renamed; `unsigned long memoryUsage` → `uint64_t memory_kb`; `unsigned long networkUsage` → `uint64_t network_kb`; ring buffers renamed; `historyIndex` → `history_idx` |
| Performance | `PerformanceData` | `TmPerfData` | Renamed; all fields to snake_case; `cpuUsage` → `cpu_percent`; `memoryUsage` → `mem_used_kb`; `gpuUsage` → `gpu_percent`; history array naming unified |
| Button | `Button` | `TmButton` | Renamed; `isHovered` → `is_hovered`; `isEnabled` → `is_enabled` |
| Tab | `Tab` | `TmTab` | Renamed; `isActive` → `is_active`; `isHovered` → `is_hovered` |
| ScrollBar | `ScrollBar` | `TmScrollBar` | Renamed; `scrollPosition` → `scroll_pos`; `isDragging` → `is_dragging`; `dragOffset` → `drag_offset` |
| *(new)* | — | `TmAppState` | Replaces all 15 module-level globals; passed by pointer to every function |
| *(new)* | — | `TmPlatform` | Vtable for OS abstraction; 5 function pointers |
| *(new)* | — | `TmObserver` | Observer callback + user_data pointer pair |
| *(new)* | — | `TmTabDescriptor` | Strategy pattern: tab id + label + draw function pointer |

---

## 14. What Was Improved & What Remains

### 14.1 Improvements Achieved

| Area | Before (ver1) | After (ver2) |
|------|--------------|-------------|
| File organization | Single 800-line .c, no headers | 16 .c + 8 .h across 6 directories |
| Global state | 15 bare module-level statics | 0 globals; all state in `TmAppState` |
| Platform isolation | `#ifdef _WIN32` in 3 locations | `#ifdef _WIN32` in exactly 1 location (main.c) |
| Error handling | `bool` returns, silent `continue` on malloc fail | `tm_result_t` enum, `TM_CHECK` macro, `tm_log_error` on failures |
| Function length | `DrawPerformanceTab()` ~95 lines; `InitializeApp()` ~65 lines | No public function exceeds 40 lines |
| Magic numbers | 21 unnamed literals throughout | All replaced with `TM_` prefixed constants in `tm_types.h` |
| Naming consistency | Mixed PascalCase/camelCase/SCREAMING with no rules | Single `module_noun_verb` convention table applied uniformly |
| Documentation | Zero comments | Doxygen headers on every file and public function; 326 comment lines |
| Build system | Manual `gcc` invocation | `CMakeLists.txt` with platform-conditional source sets |
| Design patterns | None explicitly | Strategy (tab dispatch), Command (button actions), Observer (process change) |
| Logger | `snprintf(message, ...)` + global timer | `tm_log_info/warn/error` with timestamp, level, file, line |

### 14.2 What Still Needs Work

| Area | Current State | Needed Change | Priority |
|------|--------------|---------------|----------|
| Real CPU data | `rand()` simulation | Parse `/proc/stat` deltas (Linux) or `PdhCollectQueryData` (Windows) | High |
| Real memory data | `rand()` simulation | `sysinfo()` / `/proc/meminfo` (Linux) or `GlobalMemoryStatusEx()` (Windows) | High |
| Real process metrics | Random cpu/memory per process | `/proc/[pid]/stat` parsing or `GetProcessMemoryInfo()` | High |
| Unit tests | None | Per-module tests for `tm_process.c`, `tm_startup.c`, `tm_perf.c` without raylib | Medium |
| GPU data | `rand()` simulation | NVML (NVIDIA) or DirectX DXGI (Windows) | Low |
| Configuration | Hardcoded window size, FPS | JSON or INI config file | Low |
| Memory arena | Raw `malloc`/`free` | Arena allocator mentioned in guide | Low |
| `valgrind` clean | Not verified | Run with `--leak-check=full` | Medium |

---

## 15. Lessons Learned

### 15.1 About C GUI Development with raylib

Scrollbar implementation in immediate-mode GUI (like raylib) requires computing thumb position from scratch every frame — there is no retained widget state managed by the library. In ver1 this calculation was duplicated across three `HandleScrollBarInteraction` calls for three different scroll bars. Encapsulating it in `ui_scrollbar_handle(TmScrollBar*, ...)` eliminated the duplication and made the thumb position formula a single source of truth.

Linked list traversal for rendering with a scroll offset is surprisingly subtle. The naive approach (start from the list head, skip `scrollPosition / ROW_HEIGHT` entries) visits all skipped nodes every frame — fine for 30 entries, slow for 1,000. The ver1 code does this; a future improvement would be to cache the visible-window start pointer when scroll position changes.

Cross-platform `popen` output parsing requires different format assumptions for `tasklist /fo csv /nh` (Windows CSV with quoted fields) and `ps -eo pid,comm` (space-separated). Both parsers must be written and tested independently; they share no code. The platform vtable design in ver2 makes this explicit: two files, two implementations, no shared parser logic.

Separating the "what to draw" (which tab is active) from the "how to draw" (each tab renderer) was the most valuable architectural insight. In ver1, the main loop knew about every tab renderer by name. In ver2, the main loop calls `ui_content_draw(s)` and the strategy table handles dispatch — the main loop never needs to change when a new tab is added.

### 15.2 About Code Organization in C

Header files matter even in single-person projects. Without `tm_types.h`, the `TmProcess` struct definition would either be duplicated across multiple `.c` files or implicitly assumed through global scope. The `#ifndef TM_TYPES_H` include guard ensures that a header included from multiple `.c` files produces only one definition in the final translation unit — a rule that seems obvious once you've spent 20 minutes debugging "redefinition of struct" errors.

The relationship between file organization and cognitive load is direct: when `ui_tab_performance.c` is 144 lines, a developer working on the performance graph reads 144 lines. When everything is in one 800-line file, the developer reads 800 lines to find the 144 relevant ones. This is not just aesthetics — it directly affects how quickly bugs are found and features are added.

Platform-specific code must be isolated on day one, not retrofitted. Once `#ifdef _WIN32` logic is interleaved with portable logic across 10 functions, isolating it requires understanding all 10 functions completely. Starting with the vtable structure means Windows-specific code is always in `platform_win32.c` — a rule that's easy to enforce precisely because it's a physical constraint (wrong file = won't compile with the right object).

### 15.3 About Using AI for C Refactoring

Providing the actual source code rather than a description made every AI response significantly more useful. When we described "a C task manager" generically, AI suggested generic improvements. When we pasted the actual source and referenced `DrawPerformanceTab` by name, AI identified the specific `150 + i * 30` grid line pattern and suggested exactly how to parameterize it.

Naming specific artifacts in prompts — `processScrollBar`, `cpuHistoryIndex`, `handleScrollBarInteraction` — forced AI to engage with our specific code rather than generating generic C programming advice. This was the single most effective prompting technique we discovered.

Breaking the work into 14 staged prompts produced better results than attempting one large "refactor everything" prompt. Each stage produced a concrete artifact (a naming table, a directory tree, a set of function signatures) that could be reviewed and refined before being used as input for the next stage.

AI occasionally suggested C++ idioms (`std::vector`, RAII destructors, `std::optional`). These required correction prompts ("this must be pure C99"). About 20% of generated code needed this kind of adjustment. The lesson is that AI needs explicit language version constraints, especially for C which has many dialects.

Asking AI to explain its reasoning produced better outputs than asking only for code. When we asked "explain why the platform vtable should have `open_process_list` and `parse_process_line` as separate function pointers instead of one `get_all_processes` function," the answer clarified that the two-pointer design allows line-by-line streaming without buffering the entire process list — a subtle memory management benefit we would not have thought to ask about.

---

## Appendix A: File Metrics Comparison

| Metric | ver1 | ver2 | Change |
|--------|------|------|--------|
| `.c` source files | 1 | 16 | +15 |
| `.h` header files | 0 | 8 | +8 |
| Total files | 1 | 24 | +23 |
| Directories | 1 (flat) | 6 | +5 |
| Total lines (code + docs) | ~800 | ~2,400 | +1,600 |
| Executable code lines | ~750 | ~1,100 | +350 |
| Documentation lines | 0 | ~326 | +326 |
| Public functions | 27 | ~55 | +28 |
| Static helper functions | 0 | ~35 | +35 |
| Struct definitions | 7 | 11 | +4 (`TmAppState`, `TmPlatform`, `TmObserver`, `TmTabDescriptor`) |
| Enum definitions | 1 (`TabType`) | 2 (`TmTabId`, `tm_result_t`) | +1 |
| Named constants (`#define`) | 2 (`PROCESS_CMD`, `KILL_CMD`) | 21 (`TM_ROW_HEIGHT_PX`, etc.) | +19 |
| Module-level global variables | 15 | 1 (`g_platform`) | -14 |
| `#ifdef _WIN32` locations | 3 | 1 | -2 |
| Error handling return types | `void` or `bool` | `tm_result_t` on all fallible | systematic |
| Design patterns | 0 | 3 (Strategy, Command, Observer) | +3 |
| Build system files | 0 | 1 (`CMakeLists.txt`) | +1 |

---

## Appendix B: Module Dependency Map

### B.1 Include Dependencies

Each module may only include downward in the layer hierarchy. The dependency table below lists every `#include` relationship between project files:

| File | Includes | Layer |
|------|----------|-------|
| `main.c` | `tm_types.h`, `tm_platform.h`, `tm_process.h`, `tm_perf.h`, `tm_app_history.h`, `tm_startup.h`, `tm_ui.h`, `tm_log.h` | Entry Point |
| `ui_core.c` | `tm_ui.h`, `tm_platform.h`, `tm_process.h`, `tm_startup.h`, `tm_perf.h`, `tm_log.h` | UI → Core |
| `ui_tab_processes.c` | `tm_ui.h` | UI |
| `ui_tab_performance.c` | `tm_ui.h` | UI |
| `ui_tab_history.c` | `tm_ui.h` | UI |
| `ui_tab_startup.c` | `tm_ui.h` | UI |
| `ui_scrollbar.c` | `tm_ui.h` | UI |
| `ui_button.c` | `tm_ui.h` | UI |
| `tm_process.c` | `tm_process.h`, `tm_platform.h`, `tm_log.h` | Core → Platform |
| `tm_startup.c` | `tm_startup.h`, `tm_log.h` | Core |
| `tm_app_history.c` | `tm_app_history.h`, `tm_log.h` | Core |
| `tm_perf.c` | `tm_perf.h`, `tm_platform.h`, `tm_app_history.h` | Core → Platform |
| `platform_win32.c` | `tm_platform.h`, `tm_log.h` | Platform |
| `platform_posix.c` | `tm_platform.h`, `tm_log.h` | Platform |
| `tm_log.c` | `tm_log.h` | Utils (no dependencies) |
| `tm_types.h` | `raylib.h`, `<stdbool.h>`, `<stdint.h>`, `<time.h>` | Types |

**Circular dependency check:** No module includes a file from a higher layer. `tm_process.c` (Core) does not include `tm_ui.h` (UI). `platform_posix.c` (Platform) does not include `tm_process.h` (Core). No circular includes exist.

### B.2 raylib Function Usage by Module

| raylib Function | Used In |
|----------------|---------|
| `DrawRectangle`, `DrawRectangleRec`, `DrawRectangleLines` | `ui_core.c`, all `ui_tab_*.c`, `ui_scrollbar.c`, `ui_button.c` |
| `DrawText`, `MeasureText` | `ui_core.c`, all `ui_tab_*.c` |
| `DrawLine` | `ui_tab_performance.c`, `ui_tab_history.c`, `ui_core.c` |
| `GetMousePosition` | `ui_core.c`, `ui_scrollbar.c` |
| `IsMouseButtonPressed`, `IsMouseButtonDown`, `IsMouseButtonReleased` | `ui_core.c`, `ui_scrollbar.c` |
| `CheckCollisionPointRec` | `ui_core.c`, `ui_scrollbar.c` |
| `GetMouseWheelMove` | `ui_scrollbar.c` |
| `IsKeyPressed` | `ui_core.c` |
| `SetMouseCursor` | `ui_core.c` |
| `SetWindowSize`, `GetScreenWidth`, `GetScreenHeight`, `IsWindowResized` | `ui_core.c` |
| `BeginDrawing`, `EndDrawing`, `ClearBackground` | `main.c` |
| `InitWindow`, `SetWindowState`, `SetTargetFPS`, `WindowShouldClose`, `CloseWindow` | `main.c` |
| `FLAG_WINDOW_RESIZABLE`, `MOUSE_LEFT_BUTTON`, `KEY_F5`, `KEY_DELETE` | `ui_core.c`, `main.c` |

**Note:** Zero raylib calls appear in `src/core/`, `src/platform/`, or `src/utils/`. This confirms complete separation between the graphics layer and the business/platform logic.
