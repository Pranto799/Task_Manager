cmake_minimum_required(VERSION 3.16)
project(TaskManager C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Strict warnings -- zero-warning policy
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ---- Raylib ---------------------------------------------------------------
find_package(raylib 4.0 QUIET)
if(NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.0
    )
    FetchContent_MakeAvailable(raylib)
endif()

# ---- Source files ---------------------------------------------------------
set(SOURCES
    src/main.c

    # Core (business logic -- no Raylib)
    src/core/tm_process.c
    src/core/tm_perf.c
    src/core/tm_startup.c
    src/core/tm_app_history.c

    # UI (Raylib rendering)
    src/ui/ui_core.c
    src/ui/ui_theme.c
    src/ui/ui_button.c
    src/ui/ui_scrollbar.c
    src/ui/ui_tab_processes.c
    src/ui/ui_tab_performance.c
    src/ui/ui_tab_history.c
    src/ui/ui_tab_startup.c

    # Platform adapter (OS-specific)
    src/platform/platform_posix.c
    src/platform/platform_win32.c

    # Utilities
    src/utils/tm_log.c
)

add_executable(task_manager ${SOURCES})

target_include_directories(task_manager PRIVATE include)
target_link_libraries(task_manager PRIVATE raylib)

# macOS: link system frameworks required by Raylib
if(APPLE)
    target_link_libraries(task_manager PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()
