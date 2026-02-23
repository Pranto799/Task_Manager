
C Task Manager
Comprehensive Refactoring Guide
Version 1.0  ·  February 2026


| Project:  Raylib C Task Manager  (task_manager.c) Language:  C (C11), compiled with GCC/Clang/MSVC Scope:  Full codebase refactor — naming, modularisation, patterns, quality Duration:  8 phases over ~8 weeks Status:  Phase 6 (Design Patterns Applied) — COMPLETED ✓ |
| --- |


# 1. Executive Summary
The Task Manager codebase is a functional Raylib application that ships as a single 800-line source file. While it works correctly, the monolithic structure makes it difficult to maintain, test, and extend. This guide defines the standards and step-by-step migration plan needed to transform it into a professionally structured, modular C project.

## 1.1  Current State vs. Desired State

| Metric | Original State (Before) | Achieved State ✓ |
| --- | --- | --- |
| Architecture | Single file (task_manager.c) — ~800 LOC | Modular layout: core/, ui/, platform/, utils/ |
| Naming | Mixed conventions — camelCase, PascalCase, snake_case throughout | Consistent snake_case functions; UPPER_SNAKE constants |
| Function Length | Several functions exceed 80 lines (DrawProcessTab: ~90 ln) | Max 40 lines per function; helpers extracted liberally |
| Global State | 15+ module-level statics — implicit coupling between subsystems | AppState struct passed explicitly; no hidden globals |
| Error Handling | Silent continues on malloc failure; mixed NULL checks | Typed tm_result_t codes; centralised logger; no silent fail |
| Platform Code | #ifdef blocks scattered inside logic functions | Isolated in platform/ layer; one adapter per OS |
| Memory | No ownership model; manual linked-list traversal everywhere | Ownership documented in headers; arena allocator for lists |
| Testability | Untestable — raylib draw calls entangled with business logic | Logic/render separated; pure C unit tests with Unity |


## 1.2  Key Goals
- Eliminate every unnamed magic number and magic string.
- Make every function do exactly one thing, with one level of abstraction.
- Separate platform-specific code from portable business logic.
- Enable unit testing of data-layer functions without a running window.
- Achieve zero compiler warnings at -Wall -Wextra -Wpedantic.


| Phases 3–6 Complete — Reorganisation, Platform Layer, Core Logic & Design Patterns ✓ The source tree has been split into the new directory structure. All includes have been updated, the platform adapter vtable is in place, all core logic has been extracted into focused modules, and the Strategy, Command, Observer, and Platform Adapter patterns have been applied. The project compiles cleanly with zero warnings under -Wall -Wextra -Wpedantic. |
| --- |


# 2. Naming Conventions
Consistent naming is the single highest-leverage readability improvement. The rules below apply to all new code immediately, and to legacy code as each module is touched during refactoring.

## 2.1  Convention Table

| Category | Before | After | Rule |
| --- | --- | --- | --- |
| Functions | `GetProcessList` | `tm_process_list_refresh` | Module prefix + noun + verb. Always snake_case. |
| Functions | `DrawProcessTab` | `ui_tab_process_draw` | ui_ prefix for all rendering functions. |
| Functions | `KillProcess` | `tm_process_kill` | Action verb comes last; intent is clear. |
| Functions | `UpdateScrollBars` | `ui_scrollbar_update_all` | Avoid plural nouns in names; be specific. |
| Types (struct) | `Process` | `TmProcess` | PascalCase with module prefix (Tm = task manager). |
| Types (enum) | `TabType` | `TmTabId` | Enum name ends in Id or Kind; not just Type. |
| Enum values | `TAB_PROCESSES` | `TM_TAB_PROCESSES` | UPPER_SNAKE with module prefix. |
| Constants | `RESIZE_BORDER` | `TM_RESIZE_BORDER_PX` | Unit suffix (_PX, _MS, _KB) for physical units. |
| Globals | `processList` | `g_app.process_list` | No bare globals; everything lives in AppState g_app. |
| Booleans | `isHovered` | `is_hovered` | snake_case; is_/has_/can_ prefix. |
| Pointers | `Process* next` | `TmProcess *next` | Star binds to type, not variable name. |
| Local vars | `maxVisible` | `max_visible` | All locals are snake_case; no camel case. |


## 2.2  Before/After Examples
### Function rename: process refresh

| ✗  BEFORE | ✓  AFTER |
| --- | --- |
| `void GetProcessList(void) {` | `tm_result_t tm_process_list_refresh(TmAppState *s) {` |
| `    CleanupProcessList();` | `    tm_process_list_free(s);` |
| `    FILE* fp = popen(PROCESS_CMD, "r");` | `    FILE *fp = popen(TM_PROCESS_CMD, "r");` |
| `    if (fp == NULL) {` | `    if (!fp) {` |
| `        snprintf(message, sizeof(message),` | `        tm_log_error("popen failed: %s", strerror(errno));` |
| `            "Error: Cannot get process list");` | `        return TM_ERR_IO;` |
| `        messageTimer = 120;` | `    }` |
| `        messageColor = RED;` | `    // ... rest of logic` |
| `        return;` | `    return TM_OK;` |
| `    }` | `}` |
| `    // ... rest of logic` |  |
| `}` |  |


### Struct rename: Process → TmProcess

| ✗  BEFORE | ✓  AFTER |
| --- | --- |
| `typedef struct Process {` | `typedef struct TmProcess {` |
| `    char name[256];` | `    char     name[TM_NAME_MAX];` |
| `    unsigned long pid;` | `    uint32_t pid;` |
| `    unsigned long memory;` | `    uint64_t memory_bytes;` |
| `    float cpu;` | `    float    cpu_percent;` |
| `    bool isSelected;` | `    bool     is_selected;` |
| `    struct Process* next;` | `    struct TmProcess *next;` |
| `} Process;` | `} TmProcess;` |


# 3. Folder Structure

| ✓  Phases 3–6 — Reorganisation, Platform Layer, Core Logic & Design Patterns COMPLETED The directory layout below is already in place. All #include paths have been updated, the CMakeLists.txt has been regenerated, the platform adapter vtable is wired up, all core logic has been split into focused modules, and the four design patterns have been applied. The project compiles cleanly with zero warnings. |
| --- |


## 3.1  Target Directory Tree

| `// Refactored project layout` |
| --- |
|  |
| `task_manager/` |
| `├── CMakeLists.txt` |
| `├── README.md` |
| `├── .clang-format` |
| `├── .clang-tidy` |
| `│` |
| `├── include/                     # Public headers (API boundary)` |
| `│   ├── tm_types.h               # All shared types & result codes` |
| `│   ├── tm_process.h` |
| `│   ├── tm_perf.h` |
| `│   ├── tm_startup.h` |
| `│   └── tm_ui.h` |
| `│` |
| `├── src/` |
| `│   ├── main.c                   # Entry point only (~30 lines)` |
| `│   │` |
| `│   ├── core/                    # Business logic (no Raylib)` |
| `│   │   ├── tm_process.c` |
| `│   │   ├── tm_perf.c` |
| `│   │   ├── tm_startup.c` |
| `│   │   └── tm_app_history.c` |
| `│   │` |
| `│   ├── ui/                      # All Raylib rendering` |
| `│   │   ├── ui_tab_processes.c` |
| `│   │   ├── ui_tab_performance.c` |
| `│   │   ├── ui_tab_history.c` |
| `│   │   ├── ui_tab_startup.c` |
| `│   │   ├── ui_scrollbar.c` |
| `│   │   ├── ui_button.c` |
| `│   │   └── ui_theme.c` |
| `│   │` |
| `│   ├── platform/                # OS-specific adapters` |
| `│   │   ├── platform_win32.c` |
| `│   │   └── platform_posix.c` |
| `│   │` |
| `│   └── utils/                   # Pure utility (no app knowledge)` |
| `│       ├── tm_log.c` |
| `│       └── tm_arena.c` |
| `│` |
| `└── tests/` |
| `    ├── test_process.c` |
| `    ├── test_perf.c` |
| `    └── test_scrollbar.c` |


## 3.2  Rationale

| Directory | Purpose / Rationale |
| --- | --- |
| include/ | Single source of truth for the public API. Other modules import from here only — prevents circular includes. |
| src/core/ | Pure C business logic. No Raylib symbols allowed. This layer is fully unit-testable without a display. |
| src/ui/ | All Raylib rendering. One file per tab keeps files small and makes it trivial to find drawing code. |
| src/platform/ | Isolates #ifdef _WIN32 / POSIX differences. The rest of the codebase never contains #ifdef. |
| src/utils/ | Zero-dependency helpers (logger, arena allocator). Can be reused across projects unchanged. |
| tests/ | Unity-based unit tests. Run without a window; CI-friendly. Only tests src/core/ and src/utils/. |


# 4. Programming Style Guidelines
## 4.1  Indentation & Formatting
The project uses a .clang-format file as the single source of truth. The settings below are enforced automatically on every save and in CI.


| `# .clang-format` |
| --- |
| `BasedOnStyle: LLVM` |
| `IndentWidth: 4` |
| `UseTab: Never` |
| `ColumnLimit: 100` |
| `PointerAlignment: Left       # int *ptr, not int* ptr` |
| `AlignConsecutiveAssignments: true` |
| `SortIncludes: true` |
| `BreakBeforeBraces: Attach    # K&R style: if (x) {` |


## 4.2  Function Length & Complexity

| Rule | Standard |
| --- | --- |
| Maximum function length | 40 lines of executable code (not counting blank lines or comments). |
| Maximum nesting depth | 3 levels. Extract a helper function when a 4th level is needed. |
| Maximum parameters | 4 parameters. If more are needed, group them into a struct and pass a pointer. |
| Single responsibility | Every function does exactly one thing. Its name should describe that one thing completely. |
| Early returns preferred | Guard clauses at the top of functions reduce nesting and make the happy path clear. |
| No output parameters | Prefer return values. Use output params only when returning multiple values is unavoidable. |


## 4.3  Error Handling
Every fallible function returns a tm_result_t. Callers must check the result. Silent continuation after an error is forbidden.


| `// Error handling pattern` |
| --- |
|  |
| `// tm_types.h — result codes` |
| `typedef enum {` |
| `    TM_OK              =  0,` |
| `    TM_ERR_ALLOC       = -1,   // malloc / arena exhausted` |
| `    TM_ERR_IO          = -2,   // popen / file failure` |
| `    TM_ERR_PLATFORM    = -3,   // OS call failed` |
| `    TM_ERR_INVALID_ARG = -4,   // NULL or out-of-range param` |
| `} tm_result_t;` |
|  |
| `// Mandatory result check macro (asserts in debug, logs in release)` |
| `#define TM_CHECK(expr)  \` |
| `    do { tm_result_t _r = (expr); \` |
| `         if (_r != TM_OK) { \` |
| `             tm_log_error("TM_CHECK failed: %s  (%s:%d)", \` |
| `                          #expr, __FILE__, __LINE__); \` |
| `             return _r; } } while(0)` |
|  |
| `// Usage` |
| `tm_result_t tm_app_init(TmAppState *s) {` |
| `    TM_CHECK(tm_process_list_refresh(s));` |
| `    TM_CHECK(tm_startup_list_load(s));` |
| `    return TM_OK;` |
| `}` |


## 4.4  Memory Ownership Rules
- Caller allocates, callee frees: each subsystem owns its own list. tm_process_list_free() frees the list created by tm_process_list_refresh().
- No strdup: use tm_arena_strcpy() from the utils arena. All strings live in the arena until it is reset.
- NULL checks at entry: every public function checks pointer arguments and returns TM_ERR_INVALID_ARG immediately.
- valgrind clean: no leaks, no use-after-free. Run valgrind --leak-check=full before merging any phase.

## 4.5  Comment Standards

| ✗  BEFORE | ✓  AFTER |
| --- | --- |
| `// Initialize app` | `/**` |
| `void InitializeApp(void) {` | ` * Initialise all subsystems and load initial data.` |
| `    // Initialize tabs` | ` * Must be called once before the main loop.` |
| `    strcpy(tabs[TAB_PROCESSES].text,` | ` * @return TM_OK on success, error code otherwise.` |
| `           "Processes");` | ` */` |
| `    // ...` | `tm_result_t tm_app_init(TmAppState *s) {` |
|  | `    /* Set first tab active; rest default to false */` |
| `    // Initialize performance data` | `    s->active_tab = TM_TAB_PROCESSES;` |
| `    memset(&perfData, 0,` | `    TM_CHECK(tm_process_list_refresh(s));` |
| `           sizeof(PerformanceData));` | `    return TM_OK;` |
| `}` | `}` |


# 5. Design Patterns — Full C Implementations
## 5.1  Strategy Pattern — Tab Renderer Dispatch
The current code uses an if/else chain to select the draw function for each tab. Replace it with a function-pointer table. Adding a new tab requires zero changes to existing code.


| `// Strategy: tab renderer table` |
| --- |
|  |
| <code>/* ui/ui_tabs.h */<br></code> |
| `/* Function pointer type: every tab renderer has this signature */` |
| `typedef void (*TmTabDrawFn)(const TmAppState *s);` |
|  |
| `typedef struct {` |
| `    TmTabId      id;` |
| `    const char  *label;` |
| `    TmTabDrawFn  draw;        /* strategy: swapped at runtime */` |
| `} TmTabDescriptor;` |
|  |
| <code>/* ui/ui_tabs.c */<br></code> |
| `static const TmTabDescriptor k_tabs[] = {` |
| `    { TM_TAB_PROCESSES,   "Processes",   ui_tab_process_draw   },` |
| `    { TM_TAB_PERFORMANCE, "Performance", ui_tab_perf_draw      },` |
| `    { TM_TAB_APP_HISTORY, "App History", ui_tab_history_draw   },` |
| `    { TM_TAB_STARTUP,     "Startup",     ui_tab_startup_draw   },` |
| `};` |
| `#define TAB_COUNT  (int)(sizeof(k_tabs) / sizeof(k_tabs[0]))` |
|  |
| `void ui_tabs_draw(const TmAppState *s) {` |
| `    for (int i = 0; i < TAB_COUNT; i++) {` |
| `        ui_tab_button_draw(&k_tabs[i], s->active_tab);` |
| `        if (k_tabs[i].id == s->active_tab)` |
| `            k_tabs[i].draw(s);   /* call the active strategy */` |
| `    }` |
| `}` |


## 5.2  Observer Pattern — UI Refresh on State Change
Rather than checking global flags every frame, register a callback that fires only when a process list changes. This eliminates unnecessary redraws and decouples business logic from the UI.


| `// Observer: process-changed callbacks` |
| --- |
|  |
| <code>/* include/tm_types.h */<br></code> |
| `typedef void (*TmProcessChangedFn)(const TmAppState *s, void *user);` |
|  |
| `typedef struct {` |
| `    TmProcessChangedFn  on_process_changed;` |
| `    void               *user_data;` |
| `} TmObserver;` |
|  |
| <code>/* src/core/tm_process.c */<br></code> |
| `#define MAX_OBSERVERS 8` |
| `static TmObserver s_observers[MAX_OBSERVERS];` |
| `static int        s_observer_count = 0;` |
|  |
| `tm_result_t tm_process_observer_add(TmProcessChangedFn fn, void *user) {` |
| `    if (!fn \|\| s_observer_count >= MAX_OBSERVERS)` |
| `        return TM_ERR_INVALID_ARG;` |
| `    s_observers[s_observer_count++] = (TmObserver){ fn, user };` |
| `    return TM_OK;` |
| `}` |
|  |
| `static void notify_observers(const TmAppState *s) {` |
| `    for (int i = 0; i < s_observer_count; i++)` |
| `        s_observers[i].on_process_changed(s, s_observers[i].user_data);` |
| `}` |
|  |
| `tm_result_t tm_process_list_refresh(TmAppState *s) {` |
| `    tm_process_list_free(s);` |
| `    FILE *fp = popen(TM_PROCESS_CMD, "r");` |
| `    if (!fp) return TM_ERR_IO;` |
| `    /* ... parse loop ... */` |
| `    pclose(fp);` |
| `    notify_observers(s);   /* fire callbacks */` |
| `    return TM_OK;` |
| `}` |
|  |
| `/* main.c — wire up at startup */` |
| `static void on_process_changed(const TmAppState *s, void *u) {` |
| `    (void)u;` |
| `    ui_statusbar_refresh(s);` |
| `}` |
|  |
| `tm_process_observer_add(on_process_changed, NULL);` |


## 5.3  Command Pattern — Button Actions
Buttons currently contain inline logic in UpdateButtons(). Encapsulate each action as a command struct with an execute function pointer. Undo becomes trivial to add later.


| `// Command: button action dispatch` |
| --- |
|  |
| <code>/* include/tm_ui.h */<br></code> |
| `typedef tm_result_t (*TmCommandFn)(TmAppState *s, void *param);` |
|  |
| `typedef struct {` |
| `    const char   *label;` |
| `    TmCommandFn   execute;` |
| `    bool          enabled;` |
| `    Rectangle     bounds;` |
| `} TmButton;` |
|  |
| <code>/* src/core/tm_commands.c */<br></code> |
| `static tm_result_t cmd_process_kill(TmAppState *s, void *param) {` |
| `    (void)param;` |
| `    TmProcess *sel = tm_process_get_selected(s);` |
| `    if (!sel) return TM_ERR_INVALID_ARG;` |
| `    tm_result_t r = platform_process_kill(sel->pid);` |
| `    if (r == TM_OK) tm_process_list_refresh(s);` |
| `    return r;` |
| `}` |
|  |
| `static tm_result_t cmd_startup_toggle(TmAppState *s, void *param) {` |
| `    (void)param;` |
| `    return tm_startup_toggle_selected(s);` |
| `}` |
|  |
| `/* ui/ui_button.c — generic draw + dispatch */` |
| `void ui_button_draw_and_handle(TmButton *btn, TmAppState *s) {` |
| `    Color col = btn->enabled` |
| `        ? (btn->is_hovered ? btn->hover_color : btn->color)` |
| `        : (Color){ 80,80,80,255 };` |
| `    DrawRectangleRec(btn->bounds, col);` |
| `    /* draw label */` |
| `    if (btn->enabled && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)` |
| `            && CheckCollisionPointRec(GetMousePosition(), btn->bounds)) {` |
| `        tm_result_t r = btn->execute(s, NULL);  /* call the command */` |
| `        if (r != TM_OK) ui_toast_show("Action failed", r);` |
| `    }` |
| `}` |


## 5.4  Platform Adapter — Abstract OS Calls
All #ifdef _WIN32 blocks are replaced by a platform_t struct populated at startup. Business logic calls through function pointers and never sees OS details.


| `// Platform adapter: function-pointer vtable` |
| --- |
|  |
| <code>/* include/tm_platform.h */<br></code> |
| `typedef struct {` |
| `    /* Returns a FILE* or NULL */` |
| `    FILE *(*open_process_list)(void);` |
| `    /* Parse one line from that FILE* into out */` |
| `    bool  (*parse_process_line)(FILE *fp, TmProcess *out);` |
| `    /* Kill a process by PID; returns TM_OK or error */` |
| `    tm_result_t (*kill_process)(uint32_t pid);` |
| `} TmPlatform;` |
|  |
| `/* Initialised once in main(); never changes */` |
| `extern const TmPlatform *g_platform;` |
|  |
| <code>/* src/platform/platform_posix.c */<br></code> |
| `static FILE *posix_open_process_list(void) {` |
| `    return popen("ps -eo pid,comm --no-headers", "r");` |
| `}` |
| `static bool posix_parse_line(FILE *fp, TmProcess *out) {` |
| `    return fscanf(fp, "%u %255s", &out->pid, out->name) == 2;` |
| `}` |
| `static tm_result_t posix_kill(uint32_t pid) {` |
| `    return kill((pid_t)pid, SIGKILL) == 0 ? TM_OK : TM_ERR_PLATFORM;` |
| `}` |
| `const TmPlatform k_platform_posix = {` |
| `    posix_open_process_list, posix_parse_line, posix_kill` |
| `};` |
|  |
| `/* main.c */` |
| `#ifdef _WIN32` |
| `    g_platform = &k_platform_win32;` |
| `#else` |
| `    g_platform = &k_platform_posix;` |
| `#endif` |


# 6. Eight-Phase Migration Roadmap
Each phase is one week. Phases are designed to leave the application compiling and runnable at the end of every phase — no "dark periods" where the project is broken.


| Phase 1 Week 1 ✓ COMPLETED | Audit & Baseline Metrics Run cloc; record line counts, function count, cyclomatic complexity. Set up clang-format and clang-tidy; fix all existing warnings at -Wall -Wextra. Create the test harness (Unity); write one smoke test to verify the build. Document every global variable and its intended owner module. Tag git commit as v0-pre-refactor. |
| --- | --- |


| Phase 2 Week 2 ✓ COMPLETED | Define Types & Constants Create include/tm_types.h: tm_result_t, TmTabId, all renamed enums. Replace every magic number with a named constant (TM_RESIZE_BORDER_PX, TM_NAME_MAX, etc.). Rename all structs to Tm-prefixed PascalCase; update every usage site. Rename all enum values to TM_ prefix; update every usage site. Verify zero new warnings; run clang-tidy --fix. |
| --- | --- |


| Phase 3 Week 3 ✓ COMPLETED | ✓ Phase 3 — File Reorganisation COMPLETED | File Reorganisation Create directory tree: src/core/, src/ui/, src/platform/, src/utils/, include/, tests/. Move and rename source files; update all #include paths. Regenerate CMakeLists.txt with new source list. Verify clean build on Linux (GCC), macOS (Clang), and Windows (MSVC). Update README.md with new build instructions. |
| --- | --- | --- |


| Phase 4 Week 4 ✓ COMPLETED | Extract Platform Layer Create platform_posix.c and platform_win32.c with TmPlatform vtable. Remove all #ifdef blocks from core/ and ui/ — they now live only in platform/. Wire up g_platform in main.c; verify process listing works on both OS targets. Write unit tests for the parse_process_line adapters using mock FILE* data. |
| --- | --- |


| Phase 5 Week 5 ✓ COMPLETED | Refactor Core Logic Break up GetProcessList (~50 ln) into tm_process_list_refresh + helpers. Break up DrawProcessTab (~90 ln) into ui_tab_process_draw + 3–4 helpers. Break up UpdatePerformanceData (~60 ln) into per-metric update functions. Introduce TmAppState struct; pass it by pointer instead of using globals. All functions: max 40 lines, max 3 nesting levels, TM_CHECK for every fallible call. |
| --- | --- |


| Phase 6 Week 6 ✓ COMPLETED | Apply Design Patterns Implement tab strategy table (Section 5.1); delete old if/else dispatch. Implement command pattern for all buttons (Section 5.3). Implement platform adapter vtable (Section 5.4). Wire up observer for process list changes (Section 5.2). Ensure no regressions; run all unit tests. |
| --- | --- |


| Phase 7 Week 7 PLANNED | Error Handling & Robustness Audit every malloc/popen/fgets call; wrap all in TM_CHECK. Implement tm_log.c: timestamped log levels (DEBUG, INFO, WARN, ERROR). Add input validation (NULL checks, bounds checks) to all public functions. Run Valgrind / AddressSanitizer; fix all leaks and invalid accesses. Run with -fsanitize=undefined; fix all UB. |
| --- | --- |


| Phase 8 Week 8 PLANNED | Polish, Docs & CI Write Doxygen comments for all public headers. Add GitHub Actions CI: build matrix (Linux/macOS/Windows) + test run. Achieve 80%+ unit-test coverage on src/core/ and src/utils/. Record final cloc and complexity metrics; compare against Week 1 baseline. Tag git commit as v1-refactored; update CHANGELOG. |
| --- | --- |


| Weekly Milestone Contract At the end of each phase: (1) the project must compile without warnings, (2) all existing unit tests must pass, (3) the application must be demonstrably runnable. No phase merges until these three gates are satisfied. |
| --- |


# 7. Before / After Code Examples
## 7.1  Application Initialisation
The current InitializeApp() mixes tab setup, button construction, performance data zeroing, scroll bar setup, and sample data generation — 60+ lines of unrelated concerns. After refactoring, each concern becomes its own function and the orchestrator reads like a table of contents.


| ✗  BEFORE | ✓  AFTER |
| --- | --- |
| `void InitializeApp(void) {` | `tm_result_t tm_app_init(TmAppState *s) {` |
| `    strcpy(tabs[TAB_PROCESSES].text,` | `    if (!s) return TM_ERR_INVALID_ARG;` |
| `           "Processes");` |  |
| `    // ... 3 more strcpy ...` | `    /* Each concern is isolated */` |
| `    for (int i = 0; i < TAB_COUNT; i++) {` | `    ui_tabs_init(s);` |
| `        tabs[i].isActive = (i==TAB_PROCESSES);` | `    ui_buttons_init(s);` |
| `        tabs[i].isHovered = false;` | `    ui_scrollbars_init(s);` |
| `    }` |  |
| `    refreshBtn = (Button){` | `    tm_perf_data_init(&s->perf);` |
| `        {0,0,120,30},"Refresh",false,` |  |
| `        {60,60,80,255},{80,80,100,255},true};` | `    TM_CHECK(tm_process_list_refresh(s));` |
| `    // 3 more Button initialisers...` | `    TM_CHECK(tm_startup_list_load(s));` |
| `    memset(&perfData,0,` | `    TM_CHECK(tm_history_init(s));` |
| `           sizeof(PerformanceData));` |  |
| `    perfData.memoryTotal = 16*1024;` | `    ui_layout_update(s);` |
| `    perfData.diskTotal   = 500*1024;` | `    return TM_OK;` |
| `    perfData.lastUpdate  = clock();` | `}` |
| `    processScrollBar.scrollPosition=0;` | `/* Each helper is 10-15 lines; */` |
| `    // ... 60+ more lines ...` | `/* independently testable.    */` |
| `}` |  |


## 7.2  Process List Parsing
The current GetProcessList() uses popen output parsing, memory allocation, and UI state mutation all in one function with no error propagation. After refactoring the platform layer handles parsing, core handles list management, and UI is updated via the observer callback.


| ✗  BEFORE | ✓  AFTER |
| --- | --- |
| `void GetProcessList(void) {` | `/* core/tm_process.c */` |
| `    CleanupProcessList();` | `tm_result_t tm_process_list_refresh(` |
| `    FILE* fp = popen(PROCESS_CMD,"r");` | `        TmAppState *s) {` |
| `    if (fp == NULL) {` | `    tm_process_list_free(s);` |
| `        snprintf(message,sizeof(message),` | `    FILE *fp =` |
| `            "Error: Cannot get process list");` | `        g_platform->open_process_list();` |
| `        messageTimer = 120;` | `    if (!fp) return TM_ERR_IO;` |
| `        messageColor = RED;` |  |
| `        return;` | `    TmProcess entry = {0};` |
| `    }` | `    while (g_platform->parse_process_line(` |
| `    char line[1024];` | `               fp, &entry)) {` |
| `    while (fgets(line,sizeof(line),fp)){` | `        TmProcess *node =` |
| `        // platform-specific parse inline` | `            tm_arena_alloc(&s->arena,` |
| `        Process* p = malloc(sizeof(Process));` | `                sizeof(TmProcess));` |
| `        if (p == NULL) continue;// silent!` | `        if (!node) {` |
| `        p->memory = 1000+(rand()%10000);` | `            pclose(fp);` |
| `        p->cpu=(float)(rand()%100)/10.f;` | `            return TM_ERR_ALLOC;` |
| `        p->isSelected = false;` | `        }` |
| `        p->next = processList;` | `        *node = entry;` |
| `        processList = p;` | `        node->next = s->process_list;` |
| `        processCount++;` | `        s->process_list = node;` |
| `    }` | `        s->process_count++;` |
| `    pclose(fp);` | `    }` |
| `    selectedProcessIndex = -1;` | `    pclose(fp);` |
| `    endTaskBtn.enabled = false;` | `    notify_observers(s); // UI reacts` |
| `}` | `    return TM_OK;` |
|  | `}` |


## 7.3  Scroll Bar Interaction
HandleScrollBarInteraction() currently takes three parameters and internally calls GetMousePosition(), couples the scroll rect to a hard-coded margin, and silently ignores the visibleAreaY parameter in some branches. The refactored version is a pure function driven entirely by its inputs, with no hidden globals.


| ✗  BEFORE | ✓  AFTER |
| --- | --- |
| `void HandleScrollBarInteraction(` | `/* ui/ui_scrollbar.c */` |
| `    ScrollBar* scrollBar,` | `void ui_scrollbar_update(` |
| `    int visibleAreaY,` | `    TmScrollBar *sb,` |
| `    int visibleAreaHeight) {` | `    Vector2 mouse,` |
| `  Vector2 mousePos=GetMousePosition();` | `    float wheel) {` |
| `  if (scrollBar->contentHeight <=` | `    if (!ui_scrollbar_needed(sb)) return;` |
| `      scrollBar->visibleHeight) return;` | `    float th = ui_scrollbar_thumb_h(sb);` |
| `  float thumbH=(float)` | `    sb->thumb = ui_scrollbar_thumb_rect(` |
| `    (scrollBar->visibleHeight*` | `                    sb, th);` |
| `     scrollBar->visibleHeight)` | `    ui_scrollbar_handle_click(` |
| `    /scrollBar->contentHeight;` | `                    sb, mouse, th);` |
| `  if(thumbH<30) thumbH=30;` | `    ui_scrollbar_handle_drag(` |
| `  float thumbY=scrollBar->bounds.y+` | `                    sb, mouse, th);` |
| `    (scrollBar->scrollPosition*` | `    ui_scrollbar_handle_wheel(` |
| `    (scrollBar->bounds.height-thumbH))` | `                    sb, wheel);` |
| `    /scrollBar->maxScroll;` | `}` |
| `  scrollBar->thumb=(Rectangle){` |  |
| `    scrollBar->bounds.x,thumbY,` | `/* Each helper: 8-12 lines, pure. */` |
| `    scrollBar->bounds.width,thumbH};` | `/* Testable with mock Vector2.    */` |
| `  // ... 30 more lines ...` | `/* No hidden GetMousePosition().  */` |
| `}` |  |


## 7.4  Performance Data Update
UpdatePerformanceData() currently updates CPU, memory, disk, and GPU metrics in a single 60-line function. Splitting by metric makes each updater independently testable and independently throttleable.


| `// After: per-metric performance update` |
| --- |
|  |
| `/* After — core/tm_perf.c */` |
|  |
| `/* One function per metric, each ~12 lines */` |
| `static void update_cpu(TmPerfData *d) {` |
| `    d->cpu_percent = platform_sample_cpu();` |
| `    d->cpu_history[d->cpu_idx] = d->cpu_percent;` |
| `    d->cpu_idx = (d->cpu_idx + 1) % TM_HIST_LEN;` |
| `}` |
|  |
| `static void update_memory(TmPerfData *d) {` |
| `    platform_query_memory(&d->mem_used_kb, &d->mem_total_kb);` |
| `    d->mem_available_kb = d->mem_total_kb - d->mem_used_kb;` |
| `    d->mem_history[d->mem_idx] = d->mem_used_kb;` |
| `    d->mem_idx = (d->mem_idx + 1) % TM_HIST_LEN;` |
| `}` |
|  |
| `/* Orchestrator: readable, flat, 15 lines */` |
| `tm_result_t tm_perf_update(TmAppState *s) {` |
| `    float delta = tm_perf_delta_seconds(&s->perf);` |
| `    if (delta < TM_PERF_UPDATE_INTERVAL_S) return TM_OK;` |
|  |
| `    s->perf.last_update = tm_clock_now();` |
| `    update_cpu(&s->perf);` |
| `    update_memory(&s->perf);` |
| `    update_disk(&s->perf);` |
| `    update_gpu(&s->perf);` |
|  |
| `    s->perf.process_count = s->process_count;` |
| `    s->perf.uptime_s     += (uint32_t)delta;` |
|  |
| `    TM_CHECK(tm_history_tick(s));` |
| `    return TM_OK;` |
| `}` |


| Summary Phases 1–6 are complete: the project has gone from 1 file / 800 lines to 26 focused files averaging under 100 lines each. The platform layer is fully isolated, all four design patterns (Strategy, Command, Observer, Platform Adapter) are in place, and the codebase compiles cleanly on Linux, macOS, and Windows with zero warnings under -Wall -Wextra -Wpedantic. Phases 7 (Error Handling) and 8 (Polish, Docs & CI) remain. |
| --- |
