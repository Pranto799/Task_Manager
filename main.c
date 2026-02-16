#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// PLATFORM-SPECIFIC DEFINITIONS

#ifdef _WIN32
#define PROCESS_CMD "tasklist /fo csv /nh"
#define KILL_CMD "taskkill /PID %lu /F"
#else
#define PROCESS_CMD "ps -eo pid,comm --no-headers"
#define KILL_CMD "kill -9 %lu"
#endif

// TYPE DEFINITIONS

// Tab types for different views
typedef enum
{
    TAB_PROCESSES,
    TAB_PERFORMANCE,
    TAB_APP_HISTORY,
    TAB_STARTUP,
    TAB_COUNT
} TabType;

// Process information structure
typedef struct Process
{
    char name[256];
    unsigned long pid;
    unsigned long memory;
    float cpu;
    bool isSelected;
    struct Process *next;
} Process;

// Startup application structure
typedef struct StartupApp
{
    char name[256];
    char publisher[256];
    char status[32];
    float impact;
    bool enabled;
    struct StartupApp *next;
} StartupApp;

// Application history tracking
typedef struct AppHistory
{
    char name[256];
    float cpuTime;
    float cpuTimeHistory[30];
    unsigned long memoryUsage;
    unsigned long memoryHistory[30];
    unsigned long networkUsage;
    unsigned long networkHistory[30];
    int historyIndex;
    clock_t lastUpdate;
    struct AppHistory *next;
} AppHistory;

// Performance metrics
typedef struct PerformanceData
{
    float cpuUsage;
    float cpuUsageHistory[100];
    int cpuHistoryIndex;
    unsigned long memoryUsage;
    unsigned long memoryTotal;
    unsigned long memoryAvailable;
    unsigned long memoryHistory[100];
    int memoryHistoryIndex;
    unsigned long diskUsage;
    unsigned long diskTotal;
    unsigned long diskHistory[100];
    int diskHistoryIndex;
    int processes;
    int threads;
    int uptime;
    clock_t lastUpdate;
    float gpuUsage;
    float gpuUsageHistory[100];
    int gpuHistoryIndex;
} PerformanceData;

// UI Button structure
typedef struct Button
{
    Rectangle bounds;
    char text[50];
    bool isHovered;
    Color color;
    Color hoverColor;
    bool enabled;
} Button;

// UI Tab structure
typedef struct Tab
{
    Rectangle bounds;
    char text[50];
    bool isActive;
    bool isHovered;
} Tab;

// Scrollbar structure
typedef struct ScrollBar
{
    Rectangle bounds;
    Rectangle thumb;
    bool isDragging;
    int dragOffset;
    int contentHeight;
    int visibleHeight;
    int scrollPosition;
    int maxScroll;
} ScrollBar;

// COLOR THEME

static const Color BG_COLOR = {25, 25, 35, 255};
static const Color HEADER_COLOR = {35, 35, 45, 255};
static const Color ROW_COLOR1 = {30, 30, 40, 255};
static const Color ROW_COLOR2 = {40, 40, 50, 255};
static const Color SELECTED_COLOR = {0, 80, 160, 255};
static const Color ACCENT_COLOR = {0, 150, 255, 255};
static const Color TEXT_COLOR = {240, 240, 255, 255};
static const Color SUBTLE_TEXT_COLOR = {160, 160, 180, 255};
static const Color CPU_COLOR = {46, 204, 113, 255};
static const Color MEMORY_COLOR = {52, 152, 219, 255};
static const Color DISK_COLOR = {155, 89, 182, 255};
static const Color GPU_COLOR = {231, 76, 60, 255};
static const Color ENABLED_COLOR = {46, 204, 113, 255};
static const Color DISABLED_COLOR = {231, 76, 60, 255};

// GLOBAL STATE

// Data storage
static Process *g_processList = NULL;
static StartupApp *g_startupList = NULL;
static AppHistory *g_appHistoryList = NULL;
static PerformanceData g_perfData = {0};

// UI state
static Tab g_tabs[TAB_COUNT];
static Button g_refreshBtn, g_endTaskBtn, g_enableStartupBtn, g_disableStartupBtn;
static ScrollBar g_processScrollBar, g_startupScrollBar, g_appHistoryScrollBar;

// Application state
static int g_processCount = 0;
static int g_selectedProcessIndex = -1;
static int g_selectedStartupIndex = -1;
static char g_message[256] = {0};
static int g_messageTimer = 0;
static Color g_messageColor = RED;
static int g_screenWidth = 1200;
static int g_screenHeight = 800;
static bool g_isResizing = false;
static const int RESIZE_BORDER = 8;

// FUNCTION DECLARATIONS

// Initialization
static void InitializeApplication(void);
static void UpdateUIPositions(void);

// Data Management - Process
static void ProcessList_Cleanup(void);
static void ProcessList_Refresh(void);
static bool ProcessList_KillProcess(unsigned long pid);

// Data Management - Startup
static void StartupList_Cleanup(void);
static void StartupList_Load(void);
static void StartupList_Toggle(int index);

// Data Management - App History
static void AppHistory_Cleanup(void);
static void AppHistory_Initialize(void);
static void AppHistory_Update(void);

// Data Management - Performance
static void Performance_Update(void);

// UI Components
static void UI_UpdateScrollBars(void);
static void UI_HandleScrollBarInteraction(ScrollBar *scrollBar, int visibleAreaY, int visibleAreaHeight);
static void UI_UpdateButtons(void);
static void UI_HandleButtons(void);
static void UI_DrawTabs(void);
static void UI_DrawButtons(void);
static void UI_DrawMessage(void);

// View Rendering
static void View_DrawProcesses(void);
static void View_DrawPerformance(void);
static void View_DrawAppHistory(void);
static void View_DrawStartup(void);

// Input Handling
static void Input_HandleSelection(void);
static void Input_HandleKeyboard(void);
static void Input_HandleWindowResize(void);

// Utility
static void Utility_DrawResizeHandle(void);
static bool Utility_IsMouseOverResizeHandle(void);

// INITIALIZATION FUNCTIONS

static void InitializeApplication(void)
{
    // Initialize tabs
    strcpy(g_tabs[TAB_PROCESSES].text, "Processes");
    strcpy(g_tabs[TAB_PERFORMANCE].text, "Performance");
    strcpy(g_tabs[TAB_APP_HISTORY].text, "App History");
    strcpy(g_tabs[TAB_STARTUP].text, "Startup");

    for (int i = 0; i < TAB_COUNT; i++)
    {
        g_tabs[i].isActive = (i == TAB_PROCESSES);
        g_tabs[i].isHovered = false;
    }

    // Initialize buttons
    g_refreshBtn = (Button){{0, 0, 120, 30}, "Refresh", false, (Color){60, 60, 80, 255}, (Color){80, 80, 100, 255}, true};
    g_endTaskBtn = (Button){{0, 0, 120, 30}, "End Task", false, (Color){200, 60, 60, 255}, (Color){220, 80, 80, 255}, false};
    g_enableStartupBtn = (Button){{0, 0, 140, 30}, "Enable Startup", false, (Color){60, 160, 60, 255}, (Color){80, 180, 80, 255}, false};
    g_disableStartupBtn = (Button){{0, 0, 140, 30}, "Disable Startup", false, (Color){200, 60, 60, 255}, (Color){220, 80, 80, 255}, false};

    // Initialize performance data
    memset(&g_perfData, 0, sizeof(PerformanceData));
    g_perfData.memoryTotal = 16 * 1024;
    g_perfData.diskTotal = 500 * 1024;
    g_perfData.lastUpdate = clock();

    // Initialize scroll bars
    g_processScrollBar.scrollPosition = 0;
    g_processScrollBar.isDragging = false;
    g_startupScrollBar.scrollPosition = 0;
    g_startupScrollBar.isDragging = false;
    g_appHistoryScrollBar.scrollPosition = 0;
    g_appHistoryScrollBar.isDragging = false;

    UpdateUIPositions();
    StartupList_Load();
    AppHistory_Initialize();
}

static void UpdateUIPositions(void)
{
    // Update button positions
    g_refreshBtn.bounds = (Rectangle){g_screenWidth - 180, 10, 120, 30};
    g_endTaskBtn.bounds = (Rectangle){g_screenWidth - 310, 10, 120, 30};
    g_enableStartupBtn.bounds = (Rectangle){g_screenWidth - 320, 10, 140, 30};
    g_disableStartupBtn.bounds = (Rectangle){g_screenWidth - 470, 10, 140, 30};

    // Update tab positions
    g_tabs[TAB_PROCESSES].bounds = (Rectangle){10, 50, 100, 35};
    g_tabs[TAB_PERFORMANCE].bounds = (Rectangle){115, 50, 100, 35};
    g_tabs[TAB_APP_HISTORY].bounds = (Rectangle){220, 50, 100, 35};
    g_tabs[TAB_STARTUP].bounds = (Rectangle){325, 50, 80, 35};

    // Update scrollbar positions
    g_processScrollBar.bounds = (Rectangle){g_screenWidth - 15, 120, 12, g_screenHeight - 200};
    g_processScrollBar.visibleHeight = g_screenHeight - 200;

    g_startupScrollBar.bounds = (Rectangle){g_screenWidth - 15, 220, 12, g_screenHeight - 300};
    g_startupScrollBar.visibleHeight = g_screenHeight - 300;

    g_appHistoryScrollBar.bounds = (Rectangle){g_screenWidth - 15, 230, 12, g_screenHeight - 310};
    g_appHistoryScrollBar.visibleHeight = g_screenHeight - 310;
}

// DATA MANAGEMENT - PROCESS LIST

static void ProcessList_Cleanup(void)
{
    Process *current = g_processList;
    while (current != NULL)
    {
        Process *next = current->next;
        free(current);
        current = next;
    }
    g_processList = NULL;
    g_processCount = 0;
}

static void ProcessList_Refresh(void)
{
    ProcessList_Cleanup();

    FILE *fp = popen(PROCESS_CMD, "r");
    if (fp == NULL)
    {
        snprintf(g_message, sizeof(g_message), "Error: Cannot get process list");
        g_messageTimer = 120;
        g_messageColor = RED;
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp))
    {
        char name[256] = {0};
        unsigned long pid = 1000 + g_processCount;

#ifdef _WIN32
        char *firstQuote = strchr(line, '"');
        if (firstQuote)
        {
            char *secondQuote = strchr(firstQuote + 1, '"');
            if (secondQuote)
            {
                size_t nameLen = secondQuote - firstQuote - 1;
                if (nameLen > 255)
                    nameLen = 255;
                strncpy(name, firstQuote + 1, nameLen);
                name[nameLen] = '\0';
            }
        }
#else
        sscanf(line, "%lu %255s", &pid, name);
#endif

        Process *newProc = (Process *)malloc(sizeof(Process));
        if (newProc == NULL)
            continue;

        strncpy(newProc->name, name, sizeof(newProc->name) - 1);
        newProc->name[sizeof(newProc->name) - 1] = '\0';
        newProc->pid = pid;
        newProc->memory = 1000 + (rand() % 10000);
        newProc->cpu = (float)(rand() % 100) / 10.0f;
        newProc->isSelected = false;
        newProc->next = g_processList;
        g_processList = newProc;
        g_processCount++;
    }

    pclose(fp);

    g_selectedProcessIndex = -1;
    g_endTaskBtn.enabled = false;

    strcpy(g_message, "Refreshed");
    g_messageTimer = 60;
    g_messageColor = GREEN;
}

static bool ProcessList_KillProcess(unsigned long pid)
{
    char command[256];
    snprintf(command, sizeof(command), KILL_CMD, pid);
    int result = system(command);
    return (result == 0);
}

// DATA MANAGEMENT - STARTUP LIST

static void StartupList_Cleanup(void)
{
    StartupApp *current = g_startupList;
    while (current != NULL)
    {
        StartupApp *next = current->next;
        free(current);
        current = next;
    }
    g_startupList = NULL;
}

static void StartupList_Load(void)
{
    StartupList_Cleanup();

    const char *appNames[] = {
        "Microsoft OneDrive", "Spotify", "Discord", "Steam Client",
        "Adobe Creative Cloud", "NVIDIA Display", "Realtek Audio", "Microsoft Teams"};

    const char *publishers[] = {
        "Microsoft Corporation", "Spotify AB", "Discord Inc.", "Valve Corporation",
        "Adobe Inc.", "NVIDIA Corporation", "Realtek Semiconductor", "Microsoft Corporation"};

    float impacts[] = {2.1f, 1.5f, 3.2f, 4.5f, 2.8f, 0.5f, 0.3f, 3.8f};
    bool enabled[] = {true, true, false, true, true, true, true, false};

    for (int i = 0; i < 8; i++)
    {
        StartupApp *newApp = (StartupApp *)malloc(sizeof(StartupApp));
        if (newApp == NULL)
            continue;

        strncpy(newApp->name, appNames[i], sizeof(newApp->name) - 1);
        newApp->name[sizeof(newApp->name) - 1] = '\0';

        strncpy(newApp->publisher, publishers[i], sizeof(newApp->publisher) - 1);
        newApp->publisher[sizeof(newApp->publisher) - 1] = '\0';

        strcpy(newApp->status, enabled[i] ? "Enabled" : "Disabled");
        newApp->impact = impacts[i];
        newApp->enabled = enabled[i];
        newApp->next = g_startupList;
        g_startupList = newApp;
    }
}

static void StartupList_Toggle(int index)
{
    StartupApp *current = g_startupList;
    int i = 0;

    while (current != NULL && i < index)
    {
        current = current->next;
        i++;
    }

    if (current != NULL)
    {
        current->enabled = !current->enabled;
        strcpy(current->status, current->enabled ? "Enabled" : "Disabled");

        snprintf(g_message, sizeof(g_message), "%s %s", current->name,
                 current->enabled ? "enabled" : "disabled");
        g_messageColor = current->enabled ? GREEN : ORANGE;
        g_messageTimer = 120;

        if (g_selectedStartupIndex >= 0)
        {
            g_enableStartupBtn.enabled = !current->enabled;
            g_disableStartupBtn.enabled = current->enabled;
        }
    }
}

// DATA MANAGEMENT - APP HISTORY

static void AppHistory_Cleanup(void)
{
    AppHistory *current = g_appHistoryList;
    while (current != NULL)
    {
        AppHistory *next = current->next;
        free(current);
        current = next;
    }
    g_appHistoryList = NULL;
}

static void AppHistory_Initialize(void)
{
    const char *appNames[] = {
        "chrome.exe", "Code.exe", "explorer.exe", "Spotify.exe",
        "Discord.exe", "steam.exe", "msedge.exe", "devenv.exe"};

    for (int i = 0; i < 8; i++)
    {
        AppHistory *newApp = (AppHistory *)malloc(sizeof(AppHistory));
        if (newApp == NULL)
            continue;

        strncpy(newApp->name, appNames[i], sizeof(newApp->name) - 1);
        newApp->name[sizeof(newApp->name) - 1] = '\0';
        newApp->cpuTime = 5.0f + (rand() % 50);
        newApp->memoryUsage = 100 + (rand() % 500);
        newApp->networkUsage = 10 + (rand() % 100);
        newApp->historyIndex = 0;
        newApp->lastUpdate = clock();

        for (int j = 0; j < 30; j++)
        {
            newApp->cpuTimeHistory[j] = newApp->cpuTime * (0.8f + (rand() % 40) / 100.0f);
            newApp->memoryHistory[j] = (unsigned long)(newApp->memoryUsage * (0.8f + (rand() % 40) / 100.0f));
            newApp->networkHistory[j] = (unsigned long)(newApp->networkUsage * (0.8f + (rand() % 40) / 100.0f));
        }

        newApp->next = g_appHistoryList;
        g_appHistoryList = newApp;
    }
}

static void AppHistory_Update(void)
{
    clock_t currentTime = clock();
    AppHistory *current = g_appHistoryList;

    while (current != NULL)
    {
        float deltaTime = (float)(currentTime - current->lastUpdate) / CLOCKS_PER_SEC;

        if (deltaTime > 2.0f)
        {
            current->lastUpdate = currentTime;

            current->cpuTime *= (0.9f + (rand() % 20) / 100.0f);
            current->memoryUsage = (unsigned long)(current->memoryUsage * (0.9f + (rand() % 20) / 100.0f));
            current->networkUsage = (unsigned long)(current->networkUsage * (0.8f + (rand() % 40) / 100.0f));

            current->cpuTimeHistory[current->historyIndex] = current->cpuTime;
            current->memoryHistory[current->historyIndex] = current->memoryUsage;
            current->networkHistory[current->historyIndex] = current->networkUsage;

            current->historyIndex = (current->historyIndex + 1) % 30;
        }
        current = current->next;
    }
}

// DATA MANAGEMENT - PERFORMANCE

static void Performance_Update(void)
{
    clock_t currentTime = clock();
    float deltaTime = (float)(currentTime - g_perfData.lastUpdate) / CLOCKS_PER_SEC;

    if (deltaTime < 1.0f)
        return;

    g_perfData.lastUpdate = currentTime;

    // Update CPU
    g_perfData.cpuUsage = 5.0f + (float)(rand() % 60);
    if (g_perfData.cpuUsage > 100.0f)
        g_perfData.cpuUsage = 100.0f;
    g_perfData.cpuUsageHistory[g_perfData.cpuHistoryIndex] = g_perfData.cpuUsage;
    g_perfData.cpuHistoryIndex = (g_perfData.cpuHistoryIndex + 1) % 100;

    // Update Memory
    g_perfData.memoryUsage = 4000 + (rand() % 4000);
    if (g_perfData.memoryUsage > g_perfData.memoryTotal)
    {
        g_perfData.memoryUsage = g_perfData.memoryTotal - 500;
    }
    g_perfData.memoryAvailable = g_perfData.memoryTotal - g_perfData.memoryUsage;
    g_perfData.memoryHistory[g_perfData.memoryHistoryIndex] = g_perfData.memoryUsage;
    g_perfData.memoryHistoryIndex = (g_perfData.memoryHistoryIndex + 1) % 100;

    // Update Disk
    g_perfData.diskUsage = 200000 + (rand() % 150000);
    g_perfData.diskHistory[g_perfData.diskHistoryIndex] = g_perfData.diskUsage;
    g_perfData.diskHistoryIndex = (g_perfData.diskHistoryIndex + 1) % 100;

    // Update GPU
    g_perfData.gpuUsage = 8.0f + (float)(rand() % 50);
    if (g_perfData.gpuUsage > 100.0f)
        g_perfData.gpuUsage = 100.0f;
    g_perfData.gpuUsageHistory[g_perfData.gpuHistoryIndex] = g_perfData.gpuUsage;
    g_perfData.gpuHistoryIndex = (g_perfData.gpuHistoryIndex + 1) % 100;

    // Update system info
    g_perfData.processes = g_processCount;
    g_perfData.threads = g_processCount * 3 + (rand() % 100);
    g_perfData.uptime += (int)deltaTime;

    AppHistory_Update();
}

// UI COMPONENTS - SCROLLBAR

static void UI_UpdateScrollBars(void)
{
    // Process scrollbar
    g_processScrollBar.contentHeight = g_processCount * 30;
    g_processScrollBar.maxScroll = (g_processScrollBar.contentHeight - g_processScrollBar.visibleHeight) > 0 ? g_processScrollBar.contentHeight - g_processScrollBar.visibleHeight : 0;

    // Startup scrollbar
    int startupCount = 0;
    StartupApp *currentStartup = g_startupList;
    while (currentStartup != NULL)
    {
        startupCount++;
        currentStartup = currentStartup->next;
    }
    g_startupScrollBar.contentHeight = startupCount * 45;
    g_startupScrollBar.maxScroll = (g_startupScrollBar.contentHeight - g_startupScrollBar.visibleHeight) > 0 ? g_startupScrollBar.contentHeight - g_startupScrollBar.visibleHeight : 0;

    // App history scrollbar
    int appHistoryCount = 0;
    AppHistory *currentApp = g_appHistoryList;
    while (currentApp != NULL)
    {
        appHistoryCount++;
        currentApp = currentApp->next;
    }
    g_appHistoryScrollBar.contentHeight = appHistoryCount * 65;
    g_appHistoryScrollBar.maxScroll = (g_appHistoryScrollBar.contentHeight - g_appHistoryScrollBar.visibleHeight) > 0 ? g_appHistoryScrollBar.contentHeight - g_appHistoryScrollBar.visibleHeight : 0;
}

static void UI_HandleScrollBarInteraction(ScrollBar *scrollBar, int visibleAreaY, int visibleAreaHeight)
{
    Vector2 mousePos = GetMousePosition();

    if (scrollBar->contentHeight <= scrollBar->visibleHeight)
        return;

    float thumbHeight = (float)(scrollBar->visibleHeight * scrollBar->visibleHeight) / scrollBar->contentHeight;
    if (thumbHeight < 30)
        thumbHeight = 30;

    float thumbY = scrollBar->bounds.y + (scrollBar->scrollPosition * (scrollBar->bounds.height - thumbHeight)) / scrollBar->maxScroll;
    scrollBar->thumb = (Rectangle){scrollBar->bounds.x, thumbY, scrollBar->bounds.width, thumbHeight};

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        if (CheckCollisionPointRec(mousePos, scrollBar->thumb))
        {
            scrollBar->isDragging = true;
            scrollBar->dragOffset = (int)(mousePos.y - scrollBar->thumb.y);
        }
        else if (CheckCollisionPointRec(mousePos, scrollBar->bounds))
        {
            float clickY = mousePos.y - scrollBar->bounds.y;
            float newThumbY = clickY - thumbHeight / 2;

            if (newThumbY < 0)
                newThumbY = 0;
            if (newThumbY > scrollBar->bounds.height - thumbHeight)
                newThumbY = scrollBar->bounds.height - thumbHeight;

            scrollBar->scrollPosition = (int)(newThumbY * scrollBar->maxScroll /
                                              (scrollBar->bounds.height - thumbHeight));
        }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
    {
        scrollBar->isDragging = false;
    }

    if (scrollBar->isDragging)
    {
        float newThumbY = mousePos.y - scrollBar->dragOffset;

        if (newThumbY < scrollBar->bounds.y)
            newThumbY = scrollBar->bounds.y;
        if (newThumbY > scrollBar->bounds.y + scrollBar->bounds.height - thumbHeight)
            newThumbY = scrollBar->bounds.y + scrollBar->bounds.height - thumbHeight;

        scrollBar->scrollPosition = (int)((newThumbY - scrollBar->bounds.y) * scrollBar->maxScroll /
                                          (scrollBar->bounds.height - thumbHeight));
    }

    if (CheckCollisionPointRec(mousePos, (Rectangle){10, visibleAreaY, g_screenWidth - 30, visibleAreaHeight}))
    {
        float wheel = GetMouseWheelMove();
        if (wheel != 0)
        {
            scrollBar->scrollPosition -= (int)(wheel * 30);
            if (scrollBar->scrollPosition < 0)
                scrollBar->scrollPosition = 0;
            if (scrollBar->scrollPosition > scrollBar->maxScroll)
                scrollBar->scrollPosition = scrollBar->maxScroll;
        }
    }
}

// UI COMPONENTS - BUTTONS

static void UI_UpdateButtons(void)
{
    Vector2 mousePos = GetMousePosition();

    g_refreshBtn.isHovered = CheckCollisionPointRec(mousePos, g_refreshBtn.bounds);
    g_endTaskBtn.isHovered = CheckCollisionPointRec(mousePos, g_endTaskBtn.bounds) && g_endTaskBtn.enabled;
    g_enableStartupBtn.isHovered = CheckCollisionPointRec(mousePos, g_enableStartupBtn.bounds) && g_enableStartupBtn.enabled;
    g_disableStartupBtn.isHovered = CheckCollisionPointRec(mousePos, g_disableStartupBtn.bounds) && g_disableStartupBtn.enabled;

    for (int i = 0; i < TAB_COUNT; i++)
    {
        g_tabs[i].isHovered = CheckCollisionPointRec(mousePos, g_tabs[i].bounds);
    }
}

static void UI_HandleButtons(void)
{
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        return;

    // Handle tab switching
    for (int i = 0; i < TAB_COUNT; i++)
    {
        if (g_tabs[i].isHovered)
        {
            for (int j = 0; j < TAB_COUNT; j++)
            {
                g_tabs[j].isActive = (j == i);
            }
            g_selectedProcessIndex = -1;
            g_selectedStartupIndex = -1;
            g_endTaskBtn.enabled = false;
            g_enableStartupBtn.enabled = false;
            g_disableStartupBtn.enabled = false;
            break;
        }
    }

    // Refresh button
    if (g_refreshBtn.isHovered)
    {
        ProcessList_Refresh();
        Performance_Update();
    }

    // End Task button
    if (g_endTaskBtn.isHovered && g_endTaskBtn.enabled)
    {
        Process *current = g_processList;
        int index = 0;
        while (current != NULL && index < g_selectedProcessIndex)
        {
            current = current->next;
            index++;
        }

        if (current != NULL)
        {
            if (ProcessList_KillProcess(current->pid))
            {
                strcpy(g_message, "Process terminated");
                g_messageColor = GREEN;
                ProcessList_Refresh();
            }
            else
            {
                strcpy(g_message, "Failed to terminate process");
                g_messageColor = RED;
            }
            g_messageTimer = 120;
        }
    }

    // Enable/Disable Startup buttons
    if (g_enableStartupBtn.isHovered && g_enableStartupBtn.enabled)
    {
        StartupList_Toggle(g_selectedStartupIndex);
    }

    if (g_disableStartupBtn.isHovered && g_disableStartupBtn.enabled)
    {
        StartupList_Toggle(g_selectedStartupIndex);
    }
}

// UI COMPONENTS - TABS

static void UI_DrawTabs(void)
{
    for (int i = 0; i < TAB_COUNT; i++)
    {
        Color tabColor = g_tabs[i].isActive ? ACCENT_COLOR : HEADER_COLOR;
        Color borderColor = g_tabs[i].isActive ? ACCENT_COLOR : (Color){80, 80, 80, 255};

        DrawRectangleRec(g_tabs[i].bounds, tabColor);
        DrawRectangleLines((int)g_tabs[i].bounds.x, (int)g_tabs[i].bounds.y,
                           (int)g_tabs[i].bounds.width, (int)g_tabs[i].bounds.height, borderColor);

        int textWidth = MeasureText(g_tabs[i].text, 16);
        int textX = g_tabs[i].bounds.x + (g_tabs[i].bounds.width - textWidth) / 2;
        DrawText(g_tabs[i].text, textX, g_tabs[i].bounds.y + 8, 16,
                 g_tabs[i].isActive ? WHITE : SUBTLE_TEXT_COLOR);
    }
}

static void UI_DrawButtons(void)
{
    // Refresh button
    Color refreshColor = g_refreshBtn.isHovered ? g_refreshBtn.hoverColor : g_refreshBtn.color;
    DrawRectangleRec(g_refreshBtn.bounds, refreshColor);
    DrawRectangleLines((int)g_refreshBtn.bounds.x, (int)g_refreshBtn.bounds.y,
                       (int)g_refreshBtn.bounds.width, (int)g_refreshBtn.bounds.height,
                       (Color){80, 80, 80, 255});

    int refreshTextWidth = MeasureText(g_refreshBtn.text, 14);
    DrawText(g_refreshBtn.text,
             g_refreshBtn.bounds.x + (g_refreshBtn.bounds.width - refreshTextWidth) / 2,
             g_refreshBtn.bounds.y + 7, 14, TEXT_COLOR);

    // End Task button (only in Processes tab)
    if (g_tabs[TAB_PROCESSES].isActive)
    {
        Color endTaskColor = g_endTaskBtn.enabled ? (g_endTaskBtn.isHovered ? g_endTaskBtn.hoverColor : g_endTaskBtn.color) : (Color){80, 80, 80, 255};

        DrawRectangleRec(g_endTaskBtn.bounds, endTaskColor);
        DrawRectangleLines((int)g_endTaskBtn.bounds.x, (int)g_endTaskBtn.bounds.y,
                           (int)g_endTaskBtn.bounds.width, (int)g_endTaskBtn.bounds.height,
                           (Color){80, 80, 80, 255});

        int endTaskTextWidth = MeasureText(g_endTaskBtn.text, 14);
        DrawText(g_endTaskBtn.text,
                 g_endTaskBtn.bounds.x + (g_endTaskBtn.bounds.width - endTaskTextWidth) / 2,
                 g_endTaskBtn.bounds.y + 7, 14, TEXT_COLOR);
    }

    // Startup buttons (only in Startup tab)
    if (g_tabs[TAB_STARTUP].isActive)
    {
        Color enableColor = g_enableStartupBtn.enabled ? (g_enableStartupBtn.isHovered ? g_enableStartupBtn.hoverColor : g_enableStartupBtn.color) : (Color){80, 80, 80, 255};

        DrawRectangleRec(g_enableStartupBtn.bounds, enableColor);
        DrawRectangleLines((int)g_enableStartupBtn.bounds.x, (int)g_enableStartupBtn.bounds.y,
                           (int)g_enableStartupBtn.bounds.width, (int)g_enableStartupBtn.bounds.height,
                           (Color){80, 80, 80, 255});

        int enableTextWidth = MeasureText(g_enableStartupBtn.text, 14);
        DrawText(g_enableStartupBtn.text,
                 g_enableStartupBtn.bounds.x + (g_enableStartupBtn.bounds.width - enableTextWidth) / 2,
                 g_enableStartupBtn.bounds.y + 7, 14, TEXT_COLOR);

        Color disableColor = g_disableStartupBtn.enabled ? (g_disableStartupBtn.isHovered ? g_disableStartupBtn.hoverColor : g_disableStartupBtn.color) : (Color){80, 80, 80, 255};

        DrawRectangleRec(g_disableStartupBtn.bounds, disableColor);
        DrawRectangleLines((int)g_disableStartupBtn.bounds.x, (int)g_disableStartupBtn.bounds.y,
                           (int)g_disableStartupBtn.bounds.width, (int)g_disableStartupBtn.bounds.height,
                           (Color){80, 80, 80, 255});

        int disableTextWidth = MeasureText(g_disableStartupBtn.text, 14);
        DrawText(g_disableStartupBtn.text,
                 g_disableStartupBtn.bounds.x + (g_disableStartupBtn.bounds.width - disableTextWidth) / 2,
                 g_disableStartupBtn.bounds.y + 7, 14, TEXT_COLOR);
    }
}

static void UI_DrawMessage(void)
{
    if (g_messageTimer > 0)
    {
        int textWidth = MeasureText(g_message, 16);
        int messageX = g_screenWidth / 2 - textWidth / 2;

        DrawRectangle(messageX - 10, 10, textWidth + 20, 30, g_messageColor);
        DrawRectangleLines(messageX - 10, 10, textWidth + 20, 30, (Color){100, 100, 100, 255});
        DrawText(g_message, messageX, 15, 16, WHITE);
        g_messageTimer--;
    }
}

// VIEW RENDERING - PROCESSES

static void View_DrawProcesses(void)
{
    int startY = 120;
    int contentWidth = g_screenWidth - 30;
    int processListHeight = g_screenHeight - 200;

    UI_UpdateScrollBars();
    UI_HandleScrollBarInteraction(&g_processScrollBar, startY, processListHeight);

    // Header
    DrawRectangle(10, 90, contentWidth, 25, HEADER_COLOR);
    DrawText("Name", 20, 95, 16, TEXT_COLOR);
    DrawText("PID", 300, 95, 16, TEXT_COLOR);
    DrawText("CPU", 400, 95, 16, TEXT_COLOR);
    DrawText("Memory", 500, 95, 16, TEXT_COLOR);

    // Process list
    Process *current = g_processList;
    int currentIndex = 0;
    int visibleCount = 0;
    int maxVisible = processListHeight / 30;

    while (current != NULL && visibleCount < maxVisible)
    {
        if (currentIndex >= g_processScrollBar.scrollPosition / 30)
        {
            int yPos = startY + (visibleCount * 30) - (g_processScrollBar.scrollPosition % 30);

            if (yPos >= startY && yPos < startY + processListHeight)
            {
                Color rowColor = (visibleCount % 2 == 0) ? ROW_COLOR1 : ROW_COLOR2;

                if (current->isSelected)
                {
                    rowColor = SELECTED_COLOR;
                }

                DrawRectangle(10, yPos, contentWidth, 30, rowColor);
                DrawRectangle(20, yPos + 8, 12, 12, ACCENT_COLOR);
                DrawText(current->name, 37, yPos + 8, 14, TEXT_COLOR);

                char pidText[20];
                snprintf(pidText, sizeof(pidText), "%lu", current->pid);
                DrawText(pidText, 300, yPos + 8, 14, SUBTLE_TEXT_COLOR);

                char cpuText[20];
                snprintf(cpuText, sizeof(cpuText), "%.1f%%", current->cpu);
                Color cpuColor = current->cpu > 50.0f ? (Color){255, 100, 100, 255} : SUBTLE_TEXT_COLOR;
                DrawText(cpuText, 400, yPos + 8, 14, cpuColor);

                char memText[20];
                snprintf(memText, sizeof(memText), "%.1f MB", current->memory / 1024.0f);
                DrawText(memText, 500, yPos + 8, 14, SUBTLE_TEXT_COLOR);
            }

            visibleCount++;
        }
        current = current->next;
        currentIndex++;
    }

    // Scrollbar
    if (g_processScrollBar.contentHeight > g_processScrollBar.visibleHeight)
    {
        DrawRectangleRec(g_processScrollBar.bounds, (Color){50, 50, 60, 255});
        Color scrollBarColor = g_processScrollBar.isDragging ? (Color){200, 200, 220, 255} : ACCENT_COLOR;
        DrawRectangleRec(g_processScrollBar.thumb, scrollBarColor);
    }

    // Status bar
    DrawRectangle(0, g_screenHeight - 80, g_screenWidth, 80, HEADER_COLOR);

    char statsText[256];
    snprintf(statsText, sizeof(statsText),
             "Processes: %d | CPU Usage: %.1f%% | Memory: %.1f/%.1f GB",
             g_processCount, g_perfData.cpuUsage,
             g_perfData.memoryUsage / 1024.0f, g_perfData.memoryTotal / 1024.0f);
    DrawText(statsText, 15, g_screenHeight - 65, 14, SUBTLE_TEXT_COLOR);
}

// VIEW RENDERING - PERFORMANCE

static void View_DrawPerformance(void)
{
    int contentWidth = g_screenWidth - 40;
    int leftWidth = contentWidth / 2 - 10;
    int rightWidth = contentWidth / 2 - 10;
    int rightStartX = 20 + leftWidth + 20;

    // CPU Section
    DrawText("CPU", 20, 120, 20, TEXT_COLOR);

    char cpuText[50];
    snprintf(cpuText, sizeof(cpuText), "%.1f%%", g_perfData.cpuUsage);
    DrawText(cpuText, 20 + leftWidth - MeasureText(cpuText, 24), 120, 24, TEXT_COLOR);

    DrawRectangle(20, 150, leftWidth, 120, (Color){15, 15, 20, 255});

    for (int i = 1; i < 4; i++)
    {
        int y = 150 + i * 30;
        DrawLine(20, y, 20 + leftWidth, y, (Color){40, 40, 50, 255});
    }

    for (int i = 0; i < 99; i++)
    {
        int idx1 = (g_perfData.cpuHistoryIndex + i) % 100;
        int idx2 = (g_perfData.cpuHistoryIndex + i + 1) % 100;

        float y1 = 150 + 120 - (g_perfData.cpuUsageHistory[idx1] * 120 / 100);
        float y2 = 150 + 120 - (g_perfData.cpuUsageHistory[idx2] * 120 / 100);

        float xStep = leftWidth / 100.0f;
        DrawLine(20 + i * xStep, y1, 20 + (i + 1) * xStep, y2, CPU_COLOR);
    }
    DrawRectangleLines(20, 150, leftWidth, 120, (Color){60, 60, 70, 255});

    // Memory Section
    DrawText("Memory", rightStartX, 120, 20, TEXT_COLOR);

    float memoryPercent = (float)g_perfData.memoryUsage / g_perfData.memoryTotal;
    char memText[100];
    snprintf(memText, sizeof(memText), "%.1f/%.1f GB (%.1f%%)",
             g_perfData.memoryUsage / 1024.0f,
             g_perfData.memoryTotal / 1024.0f,
             memoryPercent * 100.0f);
    DrawText(memText, rightStartX + rightWidth - MeasureText(memText, 18), 120, 18, TEXT_COLOR);

    DrawRectangle(rightStartX, 150, rightWidth, 30, (Color){40, 40, 50, 255});
    DrawRectangle(rightStartX, 150, (int)(rightWidth * memoryPercent), 30, MEMORY_COLOR);
    DrawRectangleLines(rightStartX, 150, rightWidth, 30, (Color){80, 80, 90, 255});

    DrawText("In use:", rightStartX, 190, 14, TEXT_COLOR);
    char memoryUseText[50];
    snprintf(memoryUseText, sizeof(memoryUseText), "%.1f GB", g_perfData.memoryUsage / 1024.0f);
    DrawText(memoryUseText, rightStartX + 80, 190, 14, TEXT_COLOR);

    DrawText("Available:", rightStartX, 210, 14, TEXT_COLOR);
    char memoryAvailText[50];
    snprintf(memoryAvailText, sizeof(memoryAvailText), "%.1f GB", g_perfData.memoryAvailable / 1024.0f);
    DrawText(memoryAvailText, rightStartX + 80, 210, 14, TEXT_COLOR);

    // GPU Section
    DrawText("GPU", 20, 300, 20, TEXT_COLOR);

    char gpuText[50];
    snprintf(gpuText, sizeof(gpuText), "%.1f%%", g_perfData.gpuUsage);
    DrawText(gpuText, 20 + leftWidth - MeasureText(gpuText, 24), 300, 24, TEXT_COLOR);

    DrawRectangle(20, 330, leftWidth, 80, (Color){15, 15, 20, 255});

    for (int i = 1; i < 3; i++)
    {
        int y = 330 + i * 40;
        DrawLine(20, y, 20 + leftWidth, y, (Color){40, 40, 50, 255});
    }

    for (int i = 0; i < 99; i++)
    {
        int idx1 = (g_perfData.gpuHistoryIndex + i) % 100;
        int idx2 = (g_perfData.gpuHistoryIndex + i + 1) % 100;

        float y1 = 330 + 80 - (g_perfData.gpuUsageHistory[idx1] * 80 / 100);
        float y2 = 330 + 80 - (g_perfData.gpuUsageHistory[idx2] * 80 / 100);

        float xStep = leftWidth / 100.0f;
        DrawLine(20 + i * xStep, y1, 20 + (i + 1) * xStep, y2, GPU_COLOR);
    }
    DrawRectangleLines(20, 330, leftWidth, 80, (Color){60, 60, 70, 255});

    // Disk Section
    DrawText("Disk", rightStartX, 300, 20, TEXT_COLOR);

    float diskPercent = (float)g_perfData.diskUsage / g_perfData.diskTotal;
    char diskText[100];
    snprintf(diskText, sizeof(diskText), "%.1f/%.1f GB (%.1f%%)",
             g_perfData.diskUsage / 1024.0f,
             g_perfData.diskTotal / 1024.0f,
             diskPercent * 100.0f);
    DrawText(diskText, rightStartX + rightWidth - MeasureText(diskText, 18), 300, 18, TEXT_COLOR);

    DrawRectangle(rightStartX, 330, rightWidth, 30, (Color){40, 40, 50, 255});
    DrawRectangle(rightStartX, 330, (int)(rightWidth * diskPercent), 30, DISK_COLOR);
    DrawRectangleLines(rightStartX, 330, rightWidth, 30, (Color){80, 80, 90, 255});

    // System Information
    DrawText("System Information", 20, 430, 20, TEXT_COLOR);

    char uptimeText[100];
    int hours = g_perfData.uptime / 3600;
    int minutes = (g_perfData.uptime % 3600) / 60;
    int seconds = g_perfData.uptime % 60;
    snprintf(uptimeText, sizeof(uptimeText), "Up time: %d:%02d:%02d", hours, minutes, seconds);
    DrawText(uptimeText, 20, 470, 16, SUBTLE_TEXT_COLOR);

    char processesText[50];
    snprintf(processesText, sizeof(processesText), "Processes: %d", g_perfData.processes);
    DrawText(processesText, 20, 495, 16, SUBTLE_TEXT_COLOR);

    char threadsText[50];
    snprintf(threadsText, sizeof(threadsText), "Threads: %d", g_perfData.threads);
    DrawText(threadsText, 20, 520, 16, SUBTLE_TEXT_COLOR);

    char handlesText[50];
    snprintf(handlesText, sizeof(handlesText), "Handles: %d", g_perfData.processes * 50 + 1234);
    DrawText(handlesText, 250, 470, 16, SUBTLE_TEXT_COLOR);

    char memoryText[100];
    snprintf(memoryText, sizeof(memoryText), "Physical Memory: %.1f GB", g_perfData.memoryTotal / 1024.0f);
    DrawText(memoryText, 250, 495, 16, SUBTLE_TEXT_COLOR);

    char diskTotalText[100];
    snprintf(diskTotalText, sizeof(diskTotalText), "Disk Capacity: %.1f GB", g_perfData.diskTotal / 1024.0f);
    DrawText(diskTotalText, 500, 470, 16, SUBTLE_TEXT_COLOR);
}

// VIEW RENDERING - APP HISTORY

static void View_DrawAppHistory(void)
{
    int contentWidth = g_screenWidth - 30;
    int contentHeight = g_screenHeight - 170;

    UI_UpdateScrollBars();
    UI_HandleScrollBarInteraction(&g_appHistoryScrollBar, 120, contentHeight);

    DrawRectangle(20, 120, contentWidth, contentHeight, HEADER_COLOR);
    DrawText("Application History", 30, 140, 20, TEXT_COLOR);
    DrawText("Resource usage history for applications (Last 30 samples)", 30, 170, 16, SUBTLE_TEXT_COLOR);

    DrawRectangle(20, 200, contentWidth, 25, (Color){50, 50, 60, 255});
    DrawText("Application", 30, 205, 14, TEXT_COLOR);
    DrawText("CPU Time", 250, 205, 14, TEXT_COLOR);
    DrawText("Memory", 350, 205, 14, TEXT_COLOR);
    DrawText("Network", 450, 205, 14, TEXT_COLOR);
    DrawText("History", 550, 205, 14, TEXT_COLOR);

    AppHistory *current = g_appHistoryList;
    int yPos = 230 - g_appHistoryScrollBar.scrollPosition;
    int index = 0;
    int visibleCount = 0;
    int maxVisible = (contentHeight - 30) / 65;

    while (current != NULL && visibleCount < maxVisible)
    {
        if (yPos >= 230 && yPos < 230 + contentHeight - 65)
        {
            Color rowColor = (index % 2 == 0) ? ROW_COLOR1 : ROW_COLOR2;
            DrawRectangle(20, yPos, contentWidth, 60, rowColor);

            DrawRectangle(25, yPos + 5, 12, 12, ACCENT_COLOR);
            DrawText(current->name, 45, yPos + 5, 14, TEXT_COLOR);

            char cpuText[50];
            snprintf(cpuText, sizeof(cpuText), "%.1f%%", current->cpuTime);
            DrawText(cpuText, 250, yPos + 5, 14, SUBTLE_TEXT_COLOR);

            char memText[50];
            snprintf(memText, sizeof(memText), "%.1f MB", current->memoryUsage / 1024.0f);
            DrawText(memText, 350, yPos + 5, 14, SUBTLE_TEXT_COLOR);

            char netText[50];
            snprintf(netText, sizeof(netText), "%.1f KB/s", current->networkUsage / 1024.0f);
            DrawText(netText, 450, yPos + 5, 14, SUBTLE_TEXT_COLOR);

            int graphWidth = 200;
            int graphHeight = 40;
            int graphX = 550;
            int graphY = yPos + 10;

            DrawRectangle(graphX, graphY, graphWidth, graphHeight, (Color){20, 20, 20, 255});

            float maxCpu = 0;
            for (int i = 0; i < 30; i++)
            {
                if (current->cpuTimeHistory[i] > maxCpu)
                    maxCpu = current->cpuTimeHistory[i];
            }
            if (maxCpu == 0)
                maxCpu = 1;

            for (int i = 0; i < 29; i++)
            {
                int idx1 = (current->historyIndex + i) % 30;
                int idx2 = (current->historyIndex + i + 1) % 30;

                float y1 = graphY + graphHeight - (current->cpuTimeHistory[idx1] * graphHeight / maxCpu);
                float y2 = graphY + graphHeight - (current->cpuTimeHistory[idx2] * graphHeight / maxCpu);

                float xStep = graphWidth / 30.0f;
                DrawLine(graphX + i * xStep, y1, graphX + (i + 1) * xStep, y2, CPU_COLOR);
            }
            DrawRectangleLines(graphX, graphY, graphWidth, graphHeight, (Color){80, 80, 80, 255});

            visibleCount++;
        }

        current = current->next;
        yPos += 65;
        index++;
    }

    if (g_appHistoryScrollBar.contentHeight > g_appHistoryScrollBar.visibleHeight)
    {
        DrawRectangleRec(g_appHistoryScrollBar.bounds, (Color){50, 50, 60, 255});
        Color scrollBarColor = g_appHistoryScrollBar.isDragging ? (Color){200, 200, 220, 255} : ACCENT_COLOR;
        DrawRectangleRec(g_appHistoryScrollBar.thumb, scrollBarColor);
    }
}

// VIEW RENDERING - STARTUP

static void View_DrawStartup(void)
{
    int contentWidth = g_screenWidth - 30;
    int contentHeight = g_screenHeight - 170;

    UI_UpdateScrollBars();
    UI_HandleScrollBarInteraction(&g_startupScrollBar, 120, contentHeight);

    DrawRectangle(20, 120, contentWidth, contentHeight, HEADER_COLOR);
    DrawText("Startup Applications", 30, 140, 20, TEXT_COLOR);
    DrawText("Programs that run when system starts", 30, 170, 16, SUBTLE_TEXT_COLOR);

    StartupApp *current = g_startupList;
    int index = 0;
    int yPos = 220 - g_startupScrollBar.scrollPosition;
    int visibleCount = 0;
    int maxVisible = (contentHeight - 100) / 45;

    while (current != NULL && visibleCount < maxVisible)
    {
        if (yPos >= 220 && yPos < 220 + contentHeight - 45)
        {
            Color rowColor = (index % 2 == 0) ? ROW_COLOR1 : ROW_COLOR2;

            if (index == g_selectedStartupIndex)
            {
                rowColor = SELECTED_COLOR;
            }

            DrawRectangle(30, yPos, contentWidth - 20, 40, rowColor);
            DrawRectangle(35, yPos + 12, 16, 16, ACCENT_COLOR);
            DrawText(current->name, 60, yPos + 8, 14, TEXT_COLOR);
            DrawText(current->publisher, 60, yPos + 24, 12, SUBTLE_TEXT_COLOR);

            Color statusColor = current->enabled ? ENABLED_COLOR : DISABLED_COLOR;
            DrawText(current->status, 400, yPos + 16, 14, statusColor);

            char impactText[50];
            snprintf(impactText, sizeof(impactText), "%.1f s", current->impact);
            DrawText(impactText, 500, yPos + 16, 14, SUBTLE_TEXT_COLOR);

            visibleCount++;
        }

        current = current->next;
        index++;
        yPos += 45;
    }

    if (g_startupScrollBar.contentHeight > g_startupScrollBar.visibleHeight)
    {
        DrawRectangleRec(g_startupScrollBar.bounds, (Color){50, 50, 60, 255});
        Color scrollBarColor = g_startupScrollBar.isDragging ? (Color){200, 200, 220, 255} : ACCENT_COLOR;
        DrawRectangleRec(g_startupScrollBar.thumb, scrollBarColor);
    }

    if (g_selectedStartupIndex >= 0)
    {
        StartupApp *selected = g_startupList;
        int i = 0;
        while (selected != NULL && i < g_selectedStartupIndex)
        {
            selected = selected->next;
            i++;
        }

        if (selected != NULL)
        {
            g_enableStartupBtn.enabled = !selected->enabled;
            g_disableStartupBtn.enabled = selected->enabled;
        }
    }
    else
    {
        g_enableStartupBtn.enabled = false;
        g_disableStartupBtn.enabled = false;
    }
}

// INPUT HANDLING

static void Input_HandleSelection(void)
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        Vector2 mousePos = GetMousePosition();

        if (g_tabs[TAB_PROCESSES].isActive)
        {
            if (CheckCollisionPointRec(mousePos, (Rectangle){10, 120, g_screenWidth - 30, g_screenHeight - 200}))
            {
                int clickY = (int)mousePos.y - 120 + g_processScrollBar.scrollPosition;
                int clickedIndex = clickY / 30;

                if (clickedIndex >= 0 && clickedIndex < g_processCount)
                {
                    g_selectedProcessIndex = clickedIndex;

                    Process *current = g_processList;
                    int index = 0;
                    while (current != NULL)
                    {
                        current->isSelected = (index == g_selectedProcessIndex);
                        current = current->next;
                        index++;
                    }

                    g_endTaskBtn.enabled = true;
                }
            }
        }
        else if (g_tabs[TAB_STARTUP].isActive)
        {
            if (CheckCollisionPointRec(mousePos, (Rectangle){30, 220, g_screenWidth - 60, g_screenHeight - 300}))
            {
                int clickY = (int)mousePos.y - 220 + g_startupScrollBar.scrollPosition;
                int clickedIndex = clickY / 45;

                StartupApp *current = g_startupList;
                int index = 0;
                while (current != NULL && index < clickedIndex)
                {
                    current = current->next;
                    index++;
                }

                if (current != NULL)
                {
                    g_selectedStartupIndex = clickedIndex;
                    g_enableStartupBtn.enabled = !current->enabled;
                    g_disableStartupBtn.enabled = current->enabled;
                }
            }
        }
    }
}

static void Input_HandleKeyboard(void)
{
    if (IsKeyPressed(KEY_F5))
    {
        ProcessList_Refresh();
        Performance_Update();
    }

    if (IsKeyPressed(KEY_DELETE) && g_selectedProcessIndex >= 0)
    {
        Process *current = g_processList;
        int index = 0;
        while (current != NULL && index < g_selectedProcessIndex)
        {
            current = current->next;
            index++;
        }

        if (current != NULL)
        {
            if (ProcessList_KillProcess(current->pid))
            {
                strcpy(g_message, "Process terminated");
                g_messageColor = GREEN;
                ProcessList_Refresh();
                g_messageTimer = 120;
            }
        }
    }

    if (g_selectedStartupIndex >= 0)
    {
        if (IsKeyPressed(KEY_E))
        {
            StartupList_Toggle(g_selectedStartupIndex);
        }
        if (IsKeyPressed(KEY_D))
        {
            StartupList_Toggle(g_selectedStartupIndex);
        }
    }
}

static void Input_HandleWindowResize(void)
{
    Vector2 mousePos = GetMousePosition();

    if (Utility_IsMouseOverResizeHandle())
    {
        SetMouseCursor(MOUSE_CURSOR_RESIZE_NWSE);
    }
    else
    {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && Utility_IsMouseOverResizeHandle())
    {
        g_isResizing = true;
    }

    if (g_isResizing)
    {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            int newWidth = (int)mousePos.x;
            int newHeight = (int)mousePos.y;

            if (newWidth < 800)
                newWidth = 800;
            if (newHeight < 600)
                newHeight = 600;

            SetWindowSize(newWidth, newHeight);
            g_screenWidth = newWidth;
            g_screenHeight = newHeight;

            UpdateUIPositions();
            UI_UpdateScrollBars();
        }
        else
        {
            g_isResizing = false;
        }
    }

    if (IsWindowResized() && !g_isResizing)
    {
        g_screenWidth = GetScreenWidth();
        g_screenHeight = GetScreenHeight();
        UpdateUIPositions();
        UI_UpdateScrollBars();
    }
}

// UTILITY FUNCTIONS

static bool Utility_IsMouseOverResizeHandle(void)
{
    Vector2 mousePos = GetMousePosition();
    Rectangle resizeArea = {
        g_screenWidth - RESIZE_BORDER,
        g_screenHeight - RESIZE_BORDER,
        RESIZE_BORDER,
        RESIZE_BORDER};
    return CheckCollisionPointRec(mousePos, resizeArea);
}

static void Utility_DrawResizeHandle(void)
{
    for (int i = 0; i < 3; i++)
    {
        DrawLine(g_screenWidth - 12 + i * 4, g_screenHeight - 4,
                 g_screenWidth - 4, g_screenHeight - 12 + i * 4, SUBTLE_TEXT_COLOR);
    }
}

// MAIN FUNCTION

int main(void)
{
    srand(time(NULL));

    InitWindow(1200, 800, "Advanced Task Manager - Improved");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    InitializeApplication();
    ProcessList_Refresh();

    // Main loop
    while (!WindowShouldClose())
    {
        Input_HandleWindowResize();

        UI_UpdateButtons();
        UI_HandleButtons();
        Input_HandleKeyboard();
        Input_HandleSelection();
        Performance_Update();

        BeginDrawing();
        ClearBackground(BG_COLOR);

        // Header
        DrawRectangle(0, 0, g_screenWidth, 40, ACCENT_COLOR);
        DrawText("Advanced Task Manager - Improved", 10, 10, 20, WHITE);

        // UI Components
        UI_DrawTabs();
        UI_DrawButtons();
        UI_DrawMessage();

        // Active View
        if (g_tabs[TAB_PROCESSES].isActive)
        {
            View_DrawProcesses();
        }
        else if (g_tabs[TAB_PERFORMANCE].isActive)
        {
            View_DrawPerformance();
        }
        else if (g_tabs[TAB_APP_HISTORY].isActive)
        {
            View_DrawAppHistory();
        }
        else if (g_tabs[TAB_STARTUP].isActive)
        {
            View_DrawStartup();
        }

        // Resize handle
        Utility_DrawResizeHandle();

        // Footer
        DrawRectangle(0, g_screenHeight - 80, g_screenWidth, 80, HEADER_COLOR);
        DrawText("F5: Refresh   |   Delete: End Task   |   E/D: Enable/Disable Startup",
                 15, g_screenHeight - 35, 14, SUBTLE_TEXT_COLOR);

        EndDrawing();
    }

    // Cleanup
    ProcessList_Cleanup();
    StartupList_Cleanup();
    AppHistory_Cleanup();
    CloseWindow();

    return 0;
}
