

## 📑 Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Project Overview](#2-project-overview)
3. [Original Monolithic Architecture](#3-original-monolithic-architecture)
4. [Refactored Top-Down Architecture](#4-refactored-top-down-architecture)
5. [High-Level Program Flow](#5-high-level-program-flow)
6. [Module Responsibilities](#6-module-responsibilities)
7. [State Management](#7-state-management)
8. [Data & Memory Management Strategy](#8-data--memory-management-strategy)
9. [Rendering System](#9-rendering-system)
10. [Input Handling System](#10-input-handling-system)
11. [Design Principles Applied](#11-design-principles-applied)
12. [Code Quality Improvements](#12-code-quality-improvements)
13. [Comparison: Before vs After](#13-comparison-before-vs-after)
14. [Known Limitations](#14-known-limitations)
15. [Future Improvements](#15-future-improvements)
16. [Conclusion](#16-conclusion)

---

## 1. Executive Summary

**Task Manager — Structured Refactor Edition** is a GUI-based system monitoring application developed in C using Raylib. The project demonstrates the transformation from a monolithic single-file structure into a modular, Top-Down designed architecture.

**Primary objectives of this refactor:**
- Preserve full functionality
- Improve maintainability
- Reduce global dependencies
- Apply structured programming principles

The refactoring approach follows the structured programming philosophy popularized by **Edsger W. Dijkstra** and the modular abstraction ideas promoted by **Niklaus Wirth**.

---

## 2. Project Overview

The application simulates a desktop Task Manager interface featuring:

- Process listing
- CPU & Memory performance metrics
- Startup application management
- Application history view
- Scrollable UI
- Tab-based navigation

The application runs inside a real-time loop using Raylib's rendering system.

---

## 3. Original Monolithic Architecture

Before refactoring, the project had the following characteristics:

| Issue | Description |
|---|---|
| Single file | 1000+ lines in one `.c` file |
| Mixed responsibilities | UI, logic, and data all intertwined |
| Heavy globals | Excessive global variable usage |
| Long functions | Multiple tasks handled in single functions |
| Tight coupling | Rendering and data processing were inseparable |

**Problems identified:**
- High coupling
- Low modularity
- Difficult debugging
- Hard to scale
- Poor readability

---

## 4. Refactored Top-Down Architecture

The new architecture separates responsibilities into logical modules.

### Folder Structure

```
task_manager/
├── main.c
├── core/
│   ├── app.c
│   └── app.h
├── modules/
│   ├── process_manager.c
│   ├── performance_manager.c
│   ├── startup_manager.c
│   └── history_manager.c
├── ui/
│   ├── ui_renderer.c
│   ├── ui_tabs.c
│   └── ui_scrollbar.c
├── system/
│   ├── input_handler.c
│   └── window_manager.c
└── common/
    └── app_state.h
```

Each module owns a **single responsibility** and exposes functionality through its header file.

---

## 5. High-Level Program Flow

The application follows a strict Top-Down hierarchical design.

### Level 1 — Entry Point
```
main()
├── App_Initialize()
├── App_Run()
└── App_Cleanup()
```

### Level 2 — Main Loop
```
App_Run()
└── while (!App_ShouldClose())
    ├── Input_Handle()
    ├── App_Update()
    └── App_Render()
```

### Level 3 — Update Breakdown
```
App_Update()
├── ProcessManager_Update()
├── Performance_Update()
├── StartupManager_Update()
└── HistoryManager_Update()
```

This structure ensures **hierarchical abstraction** at every level.

---

## 6. Module Responsibilities

### `core/app`
Controls the overall application lifecycle.
- Initialization
- Main loop control
- Coordinating all modules

### `modules/process_manager`
- Process data loading
- Sorting and filtering
- Process termination logic

### `modules/performance_manager`
- CPU usage simulation
- Memory usage simulation
- Periodic data updates

### `modules/startup_manager`
- Startup program list management
- Enable / Disable logic

### `modules/history_manager`
- App usage tracking
- Historical data storage

### `ui/`
- Layout drawing
- Tab navigation rendering
- Scrollbar rendering
- Text & graphics display

> Rendering logic is fully separated from data logic.

---

## 7. State Management

The system uses **centralized state management** via a shared `AppState` struct:

```c
typedef struct {
    int   activeTab;
    float cpuUsage;
    float memoryUsage;
    bool  isRunning;
} AppState;
```

All modules receive an `AppState*` pointer.

**Benefits:**
- Eliminates global variables
- Improves testability
- Clear data ownership
- Reduced hidden dependencies

---

## 8. Data & Memory Management Strategy

**Memory strategy:**
- Static allocation for data arrays
- No runtime dynamic allocation during the main loop
- Centralized state container

**This ensures:**
- Predictable memory usage
- No heap fragmentation
- Stable frame timing

---

## 9. Rendering System

**Rendering responsibilities:**
- UI layer rendering
- Tab-based content drawing
- Scroll clipping logic
- Dynamic performance graph drawing

**Rendering order:**
1. Background
2. UI layout
3. Active tab content
4. Scrollbar
5. Overlays

---

## 10. Input Handling System

Input handling is fully isolated inside `input_handler`.

**Responsibilities:**
- Keyboard events
- Mouse clicks
- Scroll wheel
- Tab switching

> Input does **not** directly modify rendering — it modifies `AppState` only.

---

## 11. Design Principles Applied

| Principle | Implementation |
|---|---|
| **Top-Down Design** | High-level control functions call lower-level functions |
| **Separation of Concerns** | Each module has a single, clear purpose |
| **Modular Programming** | Clean `.c` / `.h` separation per module |
| **Abstraction** | `main()` has no knowledge of implementation details |
| **Encapsulation** | All data passed via `AppState*` pointer |

---

## 12. Code Quality Improvements

- Renamed unclear or ambiguous functions
- Reduced 500+ line functions into focused, smaller units
- Eliminated global variables in favour of centralized state
- Introduced a structured main loop
- Improved indentation and consistent formatting
- Added uniform naming conventions across all modules
- Grouped related functions together within their modules

---

## 13. Comparison: Before vs After

| Feature | Before | After |
|---|---|---|
| File Structure | Single file | Multi-module |
| Global Variables | Many | Central `AppState` |
| Coupling | High | Low |
| Readability | Poor | Clear |
| Maintainability | Difficult | Easy |
| Scalability | Limited | High |
| Debugging | Hard | Modular |

---

## 14. Known Limitations

- No real OS-level process integration
- Simulated performance data (not live system data)
- No multithreading
- No file persistence
- UI scaling is limited

---

## 15. Future Improvements

- Real OS API integration for live process data
- Thread-based background data updates
- Save / load configuration support
- Dark / light theme switching
- Modular plugin architecture
- Improved performance graph visualization

---

## 16. Conclusion

The refactored Task Manager project demonstrates:

- Strong understanding of structured C programming
- Practical Top-Down architecture implementation
- Reduced coupling and improved modularity
- A maintainable and scalable design

The project successfully transitions from a monolithic procedural structure into a clean hierarchical architecture while preserving full original functionality. This refactoring effort serves as a practical demonstration of **software engineering principles** applied in a real-time graphical C application.

---

> *Refactoring philosophy inspired by Edsger W. Dijkstra's structured programming and Niklaus Wirth's modular abstraction principles.*
