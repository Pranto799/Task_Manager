#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Platform-specific commands
#ifdef _WIN32
    #define PROCESS_CMD "tasklist /fo csv /nh"
    #define KILL_CMD "taskkill /PID %lu /F"
#else
    #define PROCESS_CMD "ps -eo pid,comm --no-headers"
    #define KILL_CMD "kill -9 %lu"
#endif

// Types
typedef enum {
    TAB_PROCESSES,
    TAB_PERFORMANCE,
    TAB_APP_HISTORY,
    TAB_STARTUP,
    TAB_COUNT
} TabType;

typedef struct Process {
    char name[256];
    unsigned long pid;
    unsigned long memory;
    float cpu;
    bool isSelected;
    struct Process* next;
} Process;

typedef struct StartupApp {
    char name[256];
    char publisher[256];
    char status[32];
    float impact;
    bool enabled;
    struct StartupApp* next;
} StartupApp;

typedef struct AppHistory {
    char name[256];
    float cpuTime;
    float cpuTimeHistory[30];
    unsigned long memoryUsage;
    unsigned long memoryHistory[30];
    unsigned long networkUsage;
    unsigned long networkHistory[30];
    int historyIndex;
    clock_t lastUpdate;
    struct AppHistory* next;
} AppHistory;

typedef struct PerformanceData {
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

typedef struct Button {
    Rectangle bounds;
    char text[50];
    bool isHovered;
    Color color;
    Color hoverColor;
    bool enabled;
} Button;

typedef struct Tab {
    Rectangle bounds;
    char text[50];
    bool isActive;
    bool isHovered;
} Tab;

typedef struct ScrollBar {
    Rectangle bounds;
    Rectangle thumb;
    bool isDragging;
    int dragOffset;
    int contentHeight;
    int visibleHeight;
    int scrollPosition;
    int maxScroll;
} ScrollBar;

// Colors
static const Color BG_COLOR = { 25, 25, 35, 255 };
static const Color HEADER_COLOR = { 35, 35, 45, 255 };
static const Color ROW_COLOR1 = { 30, 30, 40, 255 };
static const Color ROW_COLOR2 = { 40, 40, 50, 255 };
static const Color SELECTED_COLOR = { 0, 80, 160, 255 };
static const Color ACCENT_COLOR = { 0, 150, 255, 255 };
static const Color TEXT_COLOR = { 240, 240, 255, 255 };
static const Color SUBTLE_TEXT_COLOR = { 160, 160, 180, 255 };
static const Color CPU_COLOR = { 46, 204, 113, 255 };
static const Color MEMORY_COLOR = { 52, 152, 219, 255 };
static const Color DISK_COLOR = { 155, 89, 182, 255 };
static const Color GPU_COLOR = { 231, 76, 60, 255 };
static const Color ENABLED_COLOR = { 46, 204, 113, 255 };
static const Color DISABLED_COLOR = { 231, 76, 60, 255 };

// Global variables
static Process* processList = NULL;
static StartupApp* startupList = NULL;
static AppHistory* appHistoryList = NULL;
static int processCount = 0;
static int selectedProcessIndex = -1;
static int selectedStartupIndex = -1;
static Tab tabs[TAB_COUNT];
static Button refreshBtn, endTaskBtn, enableStartupBtn, disableStartupBtn;
static char message[256] = {0};
static int messageTimer = 0;
static Color messageColor = RED;
static PerformanceData perfData;
static float cpuCoreUsage[8] = {0};
static int screenWidth = 1200;
static int screenHeight = 800;
static ScrollBar processScrollBar, startupScrollBar, appHistoryScrollBar;
static bool isResizing = false;
static const int RESIZE_BORDER = 8;

// Function declarations
static void UpdateUIElements(void);
static void InitializeApp(void);
static void CleanupProcessList(void);
static void CleanupStartupList(void);
static void CleanupAppHistory(void);
static void GetProcessList(void);
static void GetStartupApps(void);
static void UpdateAppHistory(void);
static void UpdatePerformanceData(void);
static void DrawProcessTab(void);
static void DrawPerformanceTab(void);
static void DrawAppHistoryTab(void);
static void DrawStartupTab(void);
static void DrawTabs(void);
static void DrawButtons(void);
static bool KillProcess(unsigned long pid);
static void ToggleStartupApp(int index);
static void UpdateScrollBars(void);
static void HandleScrollBarInteraction(ScrollBar* scrollBar, int visibleAreaY, int visibleAreaHeight);
static void UpdateButtons(void);
static void HandleSelection(void);
static void DrawMessage(void);
static void HandleInput(void);
static void HandleWindowResize(void);
static void DrawResizeHandle(void);
static bool IsMouseOverResizeHandle(void);

// Implementation
void UpdateUIElements(void) {
    refreshBtn.bounds = (Rectangle){ screenWidth - 180, 10, 120, 30 };
    endTaskBtn.bounds = (Rectangle){ screenWidth - 310, 10, 120, 30 };
    enableStartupBtn.bounds = (Rectangle){ screenWidth - 320, 10, 140, 30 };
    disableStartupBtn.bounds = (Rectangle){ screenWidth - 470, 10, 140, 30 };
    
    tabs[TAB_PROCESSES].bounds = (Rectangle){ 10, 50, 100, 35 };
    tabs[TAB_PERFORMANCE].bounds = (Rectangle){ 115, 50, 100, 35 };
    tabs[TAB_APP_HISTORY].bounds = (Rectangle){ 220, 50, 100, 35 };
    tabs[TAB_STARTUP].bounds = (Rectangle){ 325, 50, 80, 35 };
    
    processScrollBar.bounds = (Rectangle){ screenWidth - 15, 120, 12, screenHeight - 200 };
    processScrollBar.visibleHeight = screenHeight - 200;
    
    startupScrollBar.bounds = (Rectangle){ screenWidth - 15, 220, 12, screenHeight - 300 };
    startupScrollBar.visibleHeight = screenHeight - 300;
    
    appHistoryScrollBar.bounds = (Rectangle){ screenWidth - 15, 230, 12, screenHeight - 310 };
    appHistoryScrollBar.visibleHeight = screenHeight - 310;
}

void InitializeApp(void) {
    // Initialize tabs
    strcpy(tabs[TAB_PROCESSES].text, "Processes");
    strcpy(tabs[TAB_PERFORMANCE].text, "Performance");
    strcpy(tabs[TAB_APP_HISTORY].text, "App History");
    strcpy(tabs[TAB_STARTUP].text, "Startup");
    
    for (int i = 0; i < TAB_COUNT; i++) {
        tabs[i].isActive = (i == TAB_PROCESSES);
        tabs[i].isHovered = false;
    }
    
    // Initialize buttons
    refreshBtn = (Button){ {0, 0, 120, 30}, "Refresh", false, 
                          (Color){60, 60, 80, 255}, (Color){80, 80, 100, 255}, true };
    endTaskBtn = (Button){ {0, 0, 120, 30}, "End Task", false, 
                          (Color){200, 60, 60, 255}, (Color){220, 80, 80, 255}, false };
    enableStartupBtn = (Button){ {0, 0, 140, 30}, "Enable Startup", false,
                               (Color){60, 160, 60, 255}, (Color){80, 180, 80, 255}, false };
    disableStartupBtn = (Button){ {0, 0, 140, 30}, "Disable Startup", false,
                                (Color){200, 60, 60, 255}, (Color){220, 80, 80, 255}, false };
    
    // Initialize performance data
    memset(&perfData, 0, sizeof(PerformanceData));
    perfData.memoryTotal = 16 * 1024;
    perfData.diskTotal = 500 * 1024;
    perfData.lastUpdate = clock();
    
    // Initialize scroll bars
    processScrollBar.scrollPosition = 0;
    processScrollBar.isDragging = false;
    startupScrollBar.scrollPosition = 0;
    startupScrollBar.isDragging = false;
    appHistoryScrollBar.scrollPosition = 0;
    appHistoryScrollBar.isDragging = false;
    
    UpdateUIElements();
    GetStartupApps();
    
    // Initialize app history with sample data
    const char* appNames[] = {
        "chrome.exe", "Code.exe", "explorer.exe", "Spotify.exe",
        "Discord.exe", "steam.exe", "msedge.exe", "devenv.exe"
    };
    
    for (int i = 0; i < 8; i++) {
        AppHistory* newApp = (AppHistory*)malloc(sizeof(AppHistory));
        if (newApp == NULL) continue;
        
        strncpy(newApp->name, appNames[i], sizeof(newApp->name) - 1);
        newApp->name[sizeof(newApp->name) - 1] = '\0';
        newApp->cpuTime = 5.0f + (rand() % 50);
        newApp->memoryUsage = 100 + (rand() % 500);
        newApp->networkUsage = 10 + (rand() % 100);
        newApp->historyIndex = 0;
        newApp->lastUpdate = clock();
        
        for (int j = 0; j < 30; j++) {
            newApp->cpuTimeHistory[j] = newApp->cpuTime * (0.8f + (rand() % 40) / 100.0f);
            newApp->memoryHistory[j] = (unsigned long)(newApp->memoryUsage * (0.8f + (rand() % 40) / 100.0f));
            newApp->networkHistory[j] = (unsigned long)(newApp->networkUsage * (0.8f + (rand() % 40) / 100.0f));
        }
        
        newApp->next = appHistoryList;
        appHistoryList = newApp;
    }
}

void CleanupProcessList(void) {
    Process* current = processList;
    while (current != NULL) {
        Process* next = current->next;
        free(current);
        current = next;
    }
    processList = NULL;
    processCount = 0;
}

void CleanupStartupList(void) {
    StartupApp* current = startupList;
    while (current != NULL) {
        StartupApp* next = current->next;
        free(current);
        current = next;
    }
    startupList = NULL;
}

void CleanupAppHistory(void) {
    AppHistory* current = appHistoryList;
    while (current != NULL) {
        AppHistory* next = current->next;
        free(current);
        current = next;
    }
    appHistoryList = NULL;
}

void GetProcessList(void) {
    CleanupProcessList();
    
    FILE* fp = popen(PROCESS_CMD, "r");
    if (fp == NULL) {
        snprintf(message, sizeof(message), "Error: Cannot get process list");
        messageTimer = 120;
        messageColor = RED;
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        char name[256] = {0};
        unsigned long pid = 1000 + processCount;
        
        #ifdef _WIN32
            char* firstQuote = strchr(line, '"');
            if (firstQuote) {
                char* secondQuote = strchr(firstQuote + 1, '"');
                if (secondQuote) {
                    size_t nameLen = secondQuote - firstQuote - 1;
                    if (nameLen > 255) nameLen = 255;
                    strncpy(name, firstQuote + 1, nameLen);
                    name[nameLen] = '\0';
                }
            }
        #else
            sscanf(line, "%lu %255s", &pid, name);
        #endif

        Process* newProc = (Process*)malloc(sizeof(Process));
        if (newProc == NULL) continue;
        
        strncpy(newProc->name, name, sizeof(newProc->name) - 1);
        newProc->name[sizeof(newProc->name) - 1] = '\0';
        newProc->pid = pid;
        newProc->memory = 1000 + (rand() % 10000);
        newProc->cpu = (float)(rand() % 100) / 10.0f;
        newProc->isSelected = false;
        newProc->next = processList;
        processList = newProc;
        processCount++;
    }
    
    pclose(fp);
    
    selectedProcessIndex = -1;
    endTaskBtn.enabled = false;
    
    strcpy(message, "Refreshed");
    messageTimer = 60;
    messageColor = GREEN;
}

void GetStartupApps(void) {
    CleanupStartupList();
    
    const char* appNames[] = {
        "Microsoft OneDrive", "Spotify", "Discord", "Steam Client",
        "Adobe Creative Cloud", "NVIDIA Display", "Realtek Audio", "Microsoft Teams"
    };
    
    const char* publishers[] = {
        "Microsoft Corporation", "Spotify AB", "Discord Inc.", "Valve Corporation",
        "Adobe Inc.", "NVIDIA Corporation", "Realtek Semiconductor", "Microsoft Corporation"
    };
    
    float impacts[] = { 2.1f, 1.5f, 3.2f, 4.5f, 2.8f, 0.5f, 0.3f, 3.8f };
    bool enabled[] = { true, true, false, true, true, true, true, false };
    
    for (int i = 0; i < 8; i++) {
        StartupApp* newApp = (StartupApp*)malloc(sizeof(StartupApp));
        if (newApp == NULL) continue;
        
        strncpy(newApp->name, appNames[i], sizeof(newApp->name) - 1);
        newApp->name[sizeof(newApp->name) - 1] = '\0';
        
        strncpy(newApp->publisher, publishers[i], sizeof(newApp->publisher) - 1);
        newApp->publisher[sizeof(newApp->publisher) - 1] = '\0';
        
        strcpy(newApp->status, enabled[i] ? "Enabled" : "Disabled");
        newApp->impact = impacts[i];
        newApp->enabled = enabled[i];
        newApp->next = startupList;
        startupList = newApp;
    }
}

void UpdateAppHistory(void) {
    clock_t currentTime = clock();
    AppHistory* current = appHistoryList;
    
    while (current != NULL) {
        float deltaTime = (float)(currentTime - current->lastUpdate) / CLOCKS_PER_SEC;
        
        if (deltaTime > 2.0f) {
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

void UpdatePerformanceData(void) {
    clock_t currentTime = clock();
    float deltaTime = (float)(currentTime - perfData.lastUpdate) / CLOCKS_PER_SEC;
    
    if (deltaTime < 1.0f) return;
    
    perfData.lastUpdate = currentTime;
    
    perfData.cpuUsage = 5.0f + (float)(rand() % 60);
    if (perfData.cpuUsage > 100.0f) perfData.cpuUsage = 100.0f;
    
    perfData.cpuUsageHistory[perfData.cpuHistoryIndex] = perfData.cpuUsage;
    perfData.cpuHistoryIndex = (perfData.cpuHistoryIndex + 1) % 100;
    
    perfData.memoryUsage = 4000 + (rand() % 4000);
    if (perfData.memoryUsage > perfData.memoryTotal) {
        perfData.memoryUsage = perfData.memoryTotal - 500;
    }
    perfData.memoryAvailable = perfData.memoryTotal - perfData.memoryUsage;
    perfData.memoryHistory[perfData.memoryHistoryIndex] = perfData.memoryUsage;
    perfData.memoryHistoryIndex = (perfData.memoryHistoryIndex + 1) % 100;
    
    perfData.diskUsage = 200000 + (rand() % 150000);
    perfData.diskHistory[perfData.diskHistoryIndex] = perfData.diskUsage;
    perfData.diskHistoryIndex = (perfData.diskHistoryIndex + 1) % 100;
    
    perfData.gpuUsage = 8.0f + (float)(rand() % 50);
    if (perfData.gpuUsage > 100.0f) perfData.gpuUsage = 100.0f;
    perfData.gpuUsageHistory[perfData.gpuHistoryIndex] = perfData.gpuUsage;
    perfData.gpuHistoryIndex = (perfData.gpuHistoryIndex + 1) % 100;
    
    perfData.processes = processCount;
    perfData.threads = processCount * 3 + (rand() % 100);
    perfData.uptime += (int)deltaTime;
    
    UpdateAppHistory();
}

void UpdateScrollBars(void) {
    // Update process scroll bar
    processScrollBar.contentHeight = processCount * 30;
    processScrollBar.maxScroll = (processScrollBar.contentHeight - processScrollBar.visibleHeight) > 0 ? 
                                processScrollBar.contentHeight - processScrollBar.visibleHeight : 0;
    
    // Update startup scroll bar
    int startupCount = 0;
    StartupApp* currentStartup = startupList;
    while (currentStartup != NULL) {
        startupCount++;
        currentStartup = currentStartup->next;
    }
    
    startupScrollBar.contentHeight = startupCount * 45;
    startupScrollBar.maxScroll = (startupScrollBar.contentHeight - startupScrollBar.visibleHeight) > 0 ? 
                                startupScrollBar.contentHeight - startupScrollBar.visibleHeight : 0;
    
    // Update app history scroll bar
    int appHistoryCount = 0;
    AppHistory* currentApp = appHistoryList;
    while (currentApp != NULL) {
        appHistoryCount++;
        currentApp = currentApp->next;
    }
    
    appHistoryScrollBar.contentHeight = appHistoryCount * 65;
    appHistoryScrollBar.maxScroll = (appHistoryScrollBar.contentHeight - appHistoryScrollBar.visibleHeight) > 0 ? 
                                   appHistoryScrollBar.contentHeight - appHistoryScrollBar.visibleHeight : 0;
}

void HandleScrollBarInteraction(ScrollBar* scrollBar, int visibleAreaY, int visibleAreaHeight) {
    Vector2 mousePos = GetMousePosition();
    
    if (scrollBar->contentHeight <= scrollBar->visibleHeight) return;
    
    float thumbHeight = (float)(scrollBar->visibleHeight * scrollBar->visibleHeight) / scrollBar->contentHeight;
    if (thumbHeight < 30) thumbHeight = 30;
    
    float thumbY = scrollBar->bounds.y + (scrollBar->scrollPosition * (scrollBar->bounds.height - thumbHeight)) / scrollBar->maxScroll;
    scrollBar->thumb = (Rectangle){ scrollBar->bounds.x, thumbY, scrollBar->bounds.width, thumbHeight };
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(mousePos, scrollBar->thumb)) {
            scrollBar->isDragging = true;
            scrollBar->dragOffset = (int)(mousePos.y - scrollBar->thumb.y);
        }
        else if (CheckCollisionPointRec(mousePos, scrollBar->bounds)) {
            float clickY = mousePos.y - scrollBar->bounds.y;
            float newThumbY = clickY - thumbHeight / 2;
            
            if (newThumbY < scrollBar->bounds.y) newThumbY = scrollBar->bounds.y;
            if (newThumbY > scrollBar->bounds.y + scrollBar->bounds.height - thumbHeight) 
                newThumbY = scrollBar->bounds.y + scrollBar->bounds.height - thumbHeight;
            
            scrollBar->scrollPosition = (int)((newThumbY - scrollBar->bounds.y) * scrollBar->maxScroll / 
                                            (scrollBar->bounds.height - thumbHeight));
        }
    }
    
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        scrollBar->isDragging = false;
    }
    
    if (scrollBar->isDragging) {
        float newThumbY = mousePos.y - scrollBar->dragOffset;
        
        if (newThumbY < scrollBar->bounds.y) newThumbY = scrollBar->bounds.y;
        if (newThumbY > scrollBar->bounds.y + scrollBar->bounds.height - thumbHeight) 
            newThumbY = scrollBar->bounds.y + scrollBar->bounds.height - thumbHeight;
        
        scrollBar->scrollPosition = (int)((newThumbY - scrollBar->bounds.y) * scrollBar->maxScroll / 
                                        (scrollBar->bounds.height - thumbHeight));
    }
    
    if (CheckCollisionPointRec(mousePos, (Rectangle){ 10, visibleAreaY, screenWidth - 30, visibleAreaHeight })) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            scrollBar->scrollPosition -= (int)(wheel * 30);
            if (scrollBar->scrollPosition < 0) scrollBar->scrollPosition = 0;
            if (scrollBar->scrollPosition > scrollBar->maxScroll) scrollBar->scrollPosition = scrollBar->maxScroll;
        }
    }
}

void DrawProcessTab(void) {
    int startY = 120;
    int contentWidth = screenWidth - 30;
    int processListHeight = screenHeight - 200;
    
    UpdateScrollBars();
    HandleScrollBarInteraction(&processScrollBar, startY, processListHeight);
    
    DrawRectangle(10, 90, contentWidth, 25, HEADER_COLOR);
    DrawText("Name", 20, 95, 16, TEXT_COLOR);
    DrawText("PID", 300, 95, 16, TEXT_COLOR);
    DrawText("CPU", 400, 95, 16, TEXT_COLOR);
    DrawText("Memory", 500, 95, 16, TEXT_COLOR);
    
    Process* current = processList;
    int currentIndex = 0;
    int visibleCount = 0;
    int maxVisible = processListHeight / 30;
    
    while (current != NULL && visibleCount < maxVisible) {
        if (currentIndex >= processScrollBar.scrollPosition / 30) {
            int yPos = startY + (visibleCount * 30) - (processScrollBar.scrollPosition % 30);
            
            if (yPos >= startY && yPos < startY + processListHeight) {
                Color rowColor = (visibleCount % 2 == 0) ? ROW_COLOR1 : ROW_COLOR2;
                
                if (current->isSelected) {
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
    
    if (processScrollBar.contentHeight > processScrollBar.visibleHeight) {
        DrawRectangleRec(processScrollBar.bounds, (Color){50, 50, 60, 255});
        Color scrollBarColor = processScrollBar.isDragging ? (Color){200, 200, 220, 255} : ACCENT_COLOR;
        DrawRectangleRec(processScrollBar.thumb, scrollBarColor);
    }
    
    DrawRectangle(0, screenHeight - 80, screenWidth, 80, HEADER_COLOR);
    
    char statsText[256];
    snprintf(statsText, sizeof(statsText), 
            "Processes: %d | CPU Usage: %.1f%% | Memory: %.1f/%.1f GB", 
            processCount, perfData.cpuUsage, 
            perfData.memoryUsage / 1024.0f, perfData.memoryTotal / 1024.0f);
    DrawText(statsText, 15, screenHeight - 65, 14, SUBTLE_TEXT_COLOR);
}

void DrawPerformanceTab(void) {
    int contentWidth = screenWidth - 40;
    int leftWidth = contentWidth / 2 - 10;
    int rightWidth = contentWidth / 2 - 10;
    int rightStartX = 20 + leftWidth + 20;
    
    // CPU Section
    DrawText("CPU", 20, 120, 20, TEXT_COLOR);
    
    char cpuText[50];
    snprintf(cpuText, sizeof(cpuText), "%.1f%%", perfData.cpuUsage);
    DrawText(cpuText, 20 + leftWidth - MeasureText(cpuText, 24), 120, 24, TEXT_COLOR);
    
    DrawRectangle(20, 150, leftWidth, 120, (Color){15, 15, 20, 255});
    
    for (int i = 1; i < 4; i++) {
        int y = 150 + i * 30;
        DrawLine(20, y, 20 + leftWidth, y, (Color){40, 40, 50, 255});
    }
    
    for (int i = 0; i < 99; i++) {
        int idx1 = (perfData.cpuHistoryIndex + i) % 100;
        int idx2 = (perfData.cpuHistoryIndex + i + 1) % 100;
        
        float y1 = 150 + 120 - (perfData.cpuUsageHistory[idx1] * 120 / 100);
        float y2 = 150 + 120 - (perfData.cpuUsageHistory[idx2] * 120 / 100);
        
        float xStep = leftWidth / 100.0f;
        DrawLine(20 + i * xStep, y1, 20 + (i + 1) * xStep, y2, CPU_COLOR);
    }
    DrawRectangleLines(20, 150, leftWidth, 120, (Color){60, 60, 70, 255});
    
    // Memory Section
    DrawText("Memory", rightStartX, 120, 20, TEXT_COLOR);
    
    float memoryPercent = (float)perfData.memoryUsage / perfData.memoryTotal;
    char memText[100];
    snprintf(memText, sizeof(memText), "%.1f/%.1f GB (%.1f%%)", 
            perfData.memoryUsage / 1024.0f,
            perfData.memoryTotal / 1024.0f,
            memoryPercent * 100.0f);
    DrawText(memText, rightStartX + rightWidth - MeasureText(memText, 18), 120, 18, TEXT_COLOR);
    
    DrawRectangle(rightStartX, 150, rightWidth, 30, (Color){40, 40, 50, 255});
    DrawRectangle(rightStartX, 150, (int)(rightWidth * memoryPercent), 30, MEMORY_COLOR);
    DrawRectangleLines(rightStartX, 150, rightWidth, 30, (Color){80, 80, 90, 255});
    
    DrawText("In use:", rightStartX, 190, 14, TEXT_COLOR);
    char memoryUseText[50];
    snprintf(memoryUseText, sizeof(memoryUseText), "%.1f GB", perfData.memoryUsage / 1024.0f);
    DrawText(memoryUseText, rightStartX + 80, 190, 14, TEXT_COLOR);
    
    DrawText("Available:", rightStartX, 210, 14, TEXT_COLOR);
    char memoryAvailText[50];
    snprintf(memoryAvailText, sizeof(memoryAvailText), "%.1f GB", perfData.memoryAvailable / 1024.0f);
    DrawText(memoryAvailText, rightStartX + 80, 210, 14, TEXT_COLOR);
    
    // GPU Section
    DrawText("GPU", 20, 300, 20, TEXT_COLOR);
    
    char gpuText[50];
    snprintf(gpuText, sizeof(gpuText), "%.1f%%", perfData.gpuUsage);
    DrawText(gpuText, 20 + leftWidth - MeasureText(gpuText, 24), 300, 24, TEXT_COLOR);
    
    DrawRectangle(20, 330, leftWidth, 80, (Color){15, 15, 20, 255});
    
    for (int i = 1; i < 3; i++) {
        int y = 330 + i * 40;
        DrawLine(20, y, 20 + leftWidth, y, (Color){40, 40, 50, 255});
    }
    
    for (int i = 0; i < 99; i++) {
        int idx1 = (perfData.gpuHistoryIndex + i) % 100;
        int idx2 = (perfData.gpuHistoryIndex + i + 1) % 100;
        
        float y1 = 330 + 80 - (perfData.gpuUsageHistory[idx1] * 80 / 100);
        float y2 = 330 + 80 - (perfData.gpuUsageHistory[idx2] * 80 / 100);
        
        float xStep = leftWidth / 100.0f;
        DrawLine(20 + i * xStep, y1, 20 + (i + 1) * xStep, y2, GPU_COLOR);
    }
    DrawRectangleLines(20, 330, leftWidth, 80, (Color){60, 60, 70, 255});
    
    // Disk Section
    DrawText("Disk", rightStartX, 300, 20, TEXT_COLOR);
    
    float diskPercent = (float)perfData.diskUsage / perfData.diskTotal;
    char diskText[100];
    snprintf(diskText, sizeof(diskText), "%.1f/%.1f GB (%.1f%%)", 
            perfData.diskUsage / 1024.0f,
            perfData.diskTotal / 1024.0f,
            diskPercent * 100.0f);
    DrawText(diskText, rightStartX + rightWidth - MeasureText(diskText, 18), 300, 18, TEXT_COLOR);
    
    DrawRectangle(rightStartX, 330, rightWidth, 30, (Color){40, 40, 50, 255});
    DrawRectangle(rightStartX, 330, (int)(rightWidth * diskPercent), 30, DISK_COLOR);
    DrawRectangleLines(rightStartX, 330, rightWidth, 30, (Color){80, 80, 90, 255});
    
    // System Information Section
    DrawText("System Information", 20, 430, 20, TEXT_COLOR);
    
    char uptimeText[100];
    int hours = perfData.uptime / 3600;
    int minutes = (perfData.uptime % 3600) / 60;
    int seconds = perfData.uptime % 60;
    snprintf(uptimeText, sizeof(uptimeText), "Up time: %d:%02d:%02d", hours, minutes, seconds);
    DrawText(uptimeText, 20, 470, 16, SUBTLE_TEXT_COLOR);
    
    char processesText[50];
    snprintf(processesText, sizeof(processesText), "Processes: %d", perfData.processes);
    DrawText(processesText, 20, 495, 16, SUBTLE_TEXT_COLOR);
    
    char threadsText[50];
    snprintf(threadsText, sizeof(threadsText), "Threads: %d", perfData.threads);
    DrawText(threadsText, 20, 520, 16, SUBTLE_TEXT_COLOR);
    
    char handlesText[50];
    snprintf(handlesText, sizeof(handlesText), "Handles: %d", perfData.processes * 50 + 1234);
    DrawText(handlesText, 250, 470, 16, SUBTLE_TEXT_COLOR);
    
    char memoryText[100];
    snprintf(memoryText, sizeof(memoryText), "Physical Memory: %.1f GB", perfData.memoryTotal / 1024.0f);
    DrawText(memoryText, 250, 495, 16, SUBTLE_TEXT_COLOR);
    
    char diskTotalText[100];
    snprintf(diskTotalText, sizeof(diskTotalText), "Disk Capacity: %.1f GB", perfData.diskTotal / 1024.0f);
    DrawText(diskTotalText, 500, 470, 16, SUBTLE_TEXT_COLOR);
}

void DrawAppHistoryTab(void) {
    int contentWidth = screenWidth - 30;
    int contentHeight = screenHeight - 170;
    
    UpdateScrollBars();
    HandleScrollBarInteraction(&appHistoryScrollBar, 120, contentHeight);
    
    DrawRectangle(20, 120, contentWidth, contentHeight, HEADER_COLOR);
    DrawText("Application History", 30, 140, 20, TEXT_COLOR);
    DrawText("Resource usage history for applications (Last 30 samples)", 30, 170, 16, SUBTLE_TEXT_COLOR);
    
    DrawRectangle(20, 200, contentWidth, 25, (Color){50, 50, 60, 255});
    DrawText("Application", 30, 205, 14, TEXT_COLOR);
    DrawText("CPU Time", 250, 205, 14, TEXT_COLOR);
    DrawText("Memory", 350, 205, 14, TEXT_COLOR);
    DrawText("Network", 450, 205, 14, TEXT_COLOR);
    DrawText("History", 550, 205, 14, TEXT_COLOR);
    
    AppHistory* current = appHistoryList;
    int yPos = 230 - appHistoryScrollBar.scrollPosition;
    int index = 0;
    int visibleCount = 0;
    int maxVisible = (contentHeight - 30) / 65;
    
    while (current != NULL && visibleCount < maxVisible) {
        if (yPos >= 230 && yPos < 230 + contentHeight - 65) {
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
            for (int i = 0; i < 30; i++) {
                if (current->cpuTimeHistory[i] > maxCpu) maxCpu = current->cpuTimeHistory[i];
            }
            if (maxCpu == 0) maxCpu = 1;
            
            for (int i = 0; i < 29; i++) {
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
    
    if (appHistoryScrollBar.contentHeight > appHistoryScrollBar.visibleHeight) {
        DrawRectangleRec(appHistoryScrollBar.bounds, (Color){50, 50, 60, 255});
        Color scrollBarColor = appHistoryScrollBar.isDragging ? (Color){200, 200, 220, 255} : ACCENT_COLOR;
        DrawRectangleRec(appHistoryScrollBar.thumb, scrollBarColor);
    }
}

void DrawStartupTab(void) {
    int contentWidth = screenWidth - 30;
    int contentHeight = screenHeight - 170;
    
    UpdateScrollBars();
    HandleScrollBarInteraction(&startupScrollBar, 120, contentHeight);
    
    DrawRectangle(20, 120, contentWidth, contentHeight, HEADER_COLOR);
    DrawText("Startup Applications", 30, 140, 20, TEXT_COLOR);
    DrawText("Programs that run when system starts", 30, 170, 16, SUBTLE_TEXT_COLOR);
    
    StartupApp* current = startupList;
    int index = 0;
    int yPos = 220 - startupScrollBar.scrollPosition;
    int visibleCount = 0;
    int maxVisible = (contentHeight - 100) / 45;
    
    while (current != NULL && visibleCount < maxVisible) {
        if (yPos >= 220 && yPos < 220 + contentHeight - 45) {
            Color rowColor = (index % 2 == 0) ? ROW_COLOR1 : ROW_COLOR2;
            
            if (index == selectedStartupIndex) {
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
    
    if (startupScrollBar.contentHeight > startupScrollBar.visibleHeight) {
        DrawRectangleRec(startupScrollBar.bounds, (Color){50, 50, 60, 255});
        Color scrollBarColor = startupScrollBar.isDragging ? (Color){200, 200, 220, 255} : ACCENT_COLOR;
        DrawRectangleRec(startupScrollBar.thumb, scrollBarColor);
    }
    
    if (selectedStartupIndex >= 0) {
        StartupApp* selected = startupList;
        int i = 0;
        while (selected != NULL && i < selectedStartupIndex) {
            selected = selected->next;
            i++;
        }
        
        if (selected != NULL) {
            enableStartupBtn.enabled = !selected->enabled;
            disableStartupBtn.enabled = selected->enabled;
        }
    } else {
        enableStartupBtn.enabled = false;
        disableStartupBtn.enabled = false;
    }
}

void DrawTabs(void) {
    for (int i = 0; i < TAB_COUNT; i++) {
        Color tabColor = tabs[i].isActive ? ACCENT_COLOR : HEADER_COLOR;
        Color borderColor = tabs[i].isActive ? ACCENT_COLOR : (Color){80, 80, 80, 255};
        
        DrawRectangleRec(tabs[i].bounds, tabColor);
        DrawRectangleLines((int)tabs[i].bounds.x, (int)tabs[i].bounds.y, 
                          (int)tabs[i].bounds.width, (int)tabs[i].bounds.height, borderColor);
        
        int textWidth = MeasureText(tabs[i].text, 16);
        int textX = tabs[i].bounds.x + (tabs[i].bounds.width - textWidth) / 2;
        DrawText(tabs[i].text, textX, tabs[i].bounds.y + 8, 16, 
                tabs[i].isActive ? WHITE : SUBTLE_TEXT_COLOR);
    }
}

void DrawButtons(void) {
    // Draw refresh button
    Color refreshColor = refreshBtn.isHovered ? refreshBtn.hoverColor : refreshBtn.color;
    DrawRectangleRec(refreshBtn.bounds, refreshColor);
    DrawRectangleLines((int)refreshBtn.bounds.x, (int)refreshBtn.bounds.y, 
                      (int)refreshBtn.bounds.width, (int)refreshBtn.bounds.height, 
                      (Color){80, 80, 80, 255});
    
    int refreshTextWidth = MeasureText(refreshBtn.text, 14);
    DrawText(refreshBtn.text, 
            refreshBtn.bounds.x + (refreshBtn.bounds.width - refreshTextWidth) / 2, 
            refreshBtn.bounds.y + 7, 14, TEXT_COLOR);
    
    // Draw end task button (only in Processes tab)
    if (tabs[TAB_PROCESSES].isActive) {
        Color endTaskColor = endTaskBtn.enabled ? 
                           (endTaskBtn.isHovered ? endTaskBtn.hoverColor : endTaskBtn.color) :
                           (Color){80, 80, 80, 255};
        
        DrawRectangleRec(endTaskBtn.bounds, endTaskColor);
        DrawRectangleLines((int)endTaskBtn.bounds.x, (int)endTaskBtn.bounds.y, 
                          (int)endTaskBtn.bounds.width, (int)endTaskBtn.bounds.height, 
                          (Color){80, 80, 80, 255});
        
        int endTaskTextWidth = MeasureText(endTaskBtn.text, 14);
        DrawText(endTaskBtn.text, 
                endTaskBtn.bounds.x + (endTaskBtn.bounds.width - endTaskTextWidth) / 2, 
                endTaskBtn.bounds.y + 7, 14, TEXT_COLOR);
    }
    
    // Draw startup buttons (only in Startup tab)
    if (tabs[TAB_STARTUP].isActive) {
        Color enableColor = enableStartupBtn.enabled ? 
                          (enableStartupBtn.isHovered ? enableStartupBtn.hoverColor : enableStartupBtn.color) :
                          (Color){80, 80, 80, 255};
        
        DrawRectangleRec(enableStartupBtn.bounds, enableColor);
        DrawRectangleLines((int)enableStartupBtn.bounds.x, (int)enableStartupBtn.bounds.y, 
                          (int)enableStartupBtn.bounds.width, (int)enableStartupBtn.bounds.height, 
                          (Color){80, 80, 80, 255});
        
        int enableTextWidth = MeasureText(enableStartupBtn.text, 14);
        DrawText(enableStartupBtn.text, 
                enableStartupBtn.bounds.x + (enableStartupBtn.bounds.width - enableTextWidth) / 2, 
                enableStartupBtn.bounds.y + 7, 14, TEXT_COLOR);
        
        Color disableColor = disableStartupBtn.enabled ? 
                           (disableStartupBtn.isHovered ? disableStartupBtn.hoverColor : disableStartupBtn.color) :
                           (Color){80, 80, 80, 255};
        
        DrawRectangleRec(disableStartupBtn.bounds, disableColor);
        DrawRectangleLines((int)disableStartupBtn.bounds.x, (int)disableStartupBtn.bounds.y, 
                          (int)disableStartupBtn.bounds.width, (int)disableStartupBtn.bounds.height, 
                          (Color){80, 80, 80, 255});
        
        int disableTextWidth = MeasureText(disableStartupBtn.text, 14);
        DrawText(disableStartupBtn.text, 
                disableStartupBtn.bounds.x + (disableStartupBtn.bounds.width - disableTextWidth) / 2, 
                disableStartupBtn.bounds.y + 7, 14, TEXT_COLOR);
    }
}

bool KillProcess(unsigned long pid) {
    char command[256];
    #ifdef _WIN32
        snprintf(command, sizeof(command), KILL_CMD, pid);
    #else
        snprintf(command, sizeof(command), KILL_CMD, pid);
    #endif
    
    int result = system(command);
    return (result == 0);
}

void ToggleStartupApp(int index) {
    StartupApp* current = startupList;
    int i = 0;
    
    while (current != NULL && i < index) {
        current = current->next;
        i++;
    }
    
    if (current != NULL) {
        current->enabled = !current->enabled;
        strcpy(current->status, current->enabled ? "Enabled" : "Disabled");
        
        snprintf(message, sizeof(message), "%s %s", current->name, 
                current->enabled ? "enabled" : "disabled");
        messageColor = current->enabled ? GREEN : ORANGE;
        messageTimer = 120;
        
        if (selectedStartupIndex >= 0) {
            enableStartupBtn.enabled = !current->enabled;
            disableStartupBtn.enabled = current->enabled;
        }
    }
}

void UpdateButtons(void) {
    Vector2 mousePos = GetMousePosition();
    
    refreshBtn.isHovered = CheckCollisionPointRec(mousePos, refreshBtn.bounds);
    endTaskBtn.isHovered = CheckCollisionPointRec(mousePos, endTaskBtn.bounds) && endTaskBtn.enabled;
    enableStartupBtn.isHovered = CheckCollisionPointRec(mousePos, enableStartupBtn.bounds) && enableStartupBtn.enabled;
    disableStartupBtn.isHovered = CheckCollisionPointRec(mousePos, disableStartupBtn.bounds) && disableStartupBtn.enabled;
    
    for (int i = 0; i < TAB_COUNT; i++) {
        tabs[i].isHovered = CheckCollisionPointRec(mousePos, tabs[i].bounds);
    }
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for (int i = 0; i < TAB_COUNT; i++) {
            if (tabs[i].isHovered) {
                for (int j = 0; j < TAB_COUNT; j++) {
                    tabs[j].isActive = (j == i);
                }
                selectedProcessIndex = -1;
                selectedStartupIndex = -1;
                endTaskBtn.enabled = false;
                enableStartupBtn.enabled = false;
                disableStartupBtn.enabled = false;
                break;
            }
        }
        
        if (refreshBtn.isHovered) {
            GetProcessList();
            UpdatePerformanceData();
        }
        
        if (endTaskBtn.isHovered && endTaskBtn.enabled) {
            Process* current = processList;
            int index = 0;
            while (current != NULL && index < selectedProcessIndex) {
                current = current->next;
                index++;
            }
            
            if (current != NULL) {
                if (KillProcess(current->pid)) {
                    strcpy(message, "Process terminated");
                    messageColor = GREEN;
                    GetProcessList();
                } else {
                    strcpy(message, "Failed to terminate process");
                    messageColor = RED;
                }
                messageTimer = 120;
            }
        }
        
        if (enableStartupBtn.isHovered && enableStartupBtn.enabled) {
            ToggleStartupApp(selectedStartupIndex);
        }
        
        if (disableStartupBtn.isHovered && disableStartupBtn.enabled) {
            ToggleStartupApp(selectedStartupIndex);
        }
    }
}

void HandleSelection(void) {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        
        if (tabs[TAB_PROCESSES].isActive) {
            if (CheckCollisionPointRec(mousePos, (Rectangle){10, 120, screenWidth - 30, screenHeight - 200})) {
                int clickY = (int)mousePos.y - 120 + processScrollBar.scrollPosition;
                int clickedIndex = clickY / 30;
                
                if (clickedIndex >= 0 && clickedIndex < processCount) {
                    selectedProcessIndex = clickedIndex;
                    
                    Process* current = processList;
                    int index = 0;
                    while (current != NULL) {
                        current->isSelected = (index == selectedProcessIndex);
                        current = current->next;
                        index++;
                    }
                    
                    endTaskBtn.enabled = true;
                }
            }
        }
        else if (tabs[TAB_STARTUP].isActive) {
            if (CheckCollisionPointRec(mousePos, (Rectangle){30, 220, screenWidth - 60, screenHeight - 300})) {
                int clickY = (int)mousePos.y - 220 + startupScrollBar.scrollPosition;
                int clickedIndex = clickY / 45;
                
                StartupApp* current = startupList;
                int index = 0;
                while (current != NULL && index < clickedIndex) {
                    current = current->next;
                    index++;
                }
                
                if (current != NULL) {
                    selectedStartupIndex = clickedIndex;
                    enableStartupBtn.enabled = !current->enabled;
                    disableStartupBtn.enabled = current->enabled;
                }
            }
        }
    }
}

void DrawMessage(void) {
    if (messageTimer > 0) {
        int textWidth = MeasureText(message, 16);
        int messageX = screenWidth/2 - textWidth/2;
        
        DrawRectangle(messageX - 10, 10, textWidth + 20, 30, messageColor);
        DrawRectangleLines(messageX - 10, 10, textWidth + 20, 30, (Color){100, 100, 100, 255});
        DrawText(message, messageX, 15, 16, WHITE);
        messageTimer--;
    }
}

void HandleInput(void) {
    if (IsKeyPressed(KEY_F5)) {
        GetProcessList();
        UpdatePerformanceData();
    }
    
    if (IsKeyPressed(KEY_DELETE) && selectedProcessIndex >= 0) {
        Process* current = processList;
        int index = 0;
        while (current != NULL && index < selectedProcessIndex) {
            current = current->next;
            index++;
        }
        
        if (current != NULL) {
            if (KillProcess(current->pid)) {
                strcpy(message, "Process terminated");
                messageColor = GREEN;
                GetProcessList();
                messageTimer = 120;
            }
        }
    }
    
    if (selectedStartupIndex >= 0) {
        if (IsKeyPressed(KEY_E)) {
            ToggleStartupApp(selectedStartupIndex);
        }
        if (IsKeyPressed(KEY_D)) {
            ToggleStartupApp(selectedStartupIndex);
        }
    }
}

bool IsMouseOverResizeHandle(void) {
    Vector2 mousePos = GetMousePosition();
    Rectangle resizeArea = {
        screenWidth - RESIZE_BORDER,
        screenHeight - RESIZE_BORDER,
        RESIZE_BORDER,
        RESIZE_BORDER
    };
    return CheckCollisionPointRec(mousePos, resizeArea);
}

void HandleWindowResize(void) {
    Vector2 mousePos = GetMousePosition();
    
    if (IsMouseOverResizeHandle()) {
        SetMouseCursor(MOUSE_CURSOR_RESIZE_NWSE);
    } else {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsMouseOverResizeHandle()) {
        isResizing = true;
    }
    
    if (isResizing) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            int newWidth = (int)mousePos.x;
            int newHeight = (int)mousePos.y;
            
            if (newWidth < 800) newWidth = 800;
            if (newHeight < 600) newHeight = 600;
            
            SetWindowSize(newWidth, newHeight);
            screenWidth = newWidth;
            screenHeight = newHeight;
            
            UpdateUIElements();
            UpdateScrollBars();
        } else {
            isResizing = false;
        }
    }
    
    if (IsWindowResized() && !isResizing) {
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        UpdateUIElements();
        UpdateScrollBars();
    }
}

void DrawResizeHandle(void) {
    for (int i = 0; i < 3; i++) {
        DrawLine(screenWidth - 12 + i*4, screenHeight - 4, screenWidth - 4, screenHeight - 12 + i*4, SUBTLE_TEXT_COLOR);
    }
}

int main(void) {
    InitWindow(1200, 800, "Advanced Task Manager");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);
    
    InitializeApp();
    GetProcessList();
    
    while (!WindowShouldClose()) {
        HandleWindowResize();
        
        UpdateButtons();
        HandleInput();
        HandleSelection();
        UpdatePerformanceData();
        
        BeginDrawing();
        ClearBackground(BG_COLOR);
        
        DrawRectangle(0, 0, screenWidth, 40, ACCENT_COLOR);
        DrawText("Advanced Task Manager", 10, 10, 20, WHITE);
        
        DrawTabs();
        DrawButtons();
        DrawMessage();
        
        if (tabs[TAB_PROCESSES].isActive) {
            DrawProcessTab();
        } else if (tabs[TAB_PERFORMANCE].isActive) {
            DrawPerformanceTab();
        } else if (tabs[TAB_APP_HISTORY].isActive) {
            DrawAppHistoryTab();
        } else if (tabs[TAB_STARTUP].isActive) {
            DrawStartupTab();
        }
        
        DrawResizeHandle();
        
        DrawRectangle(0, screenHeight - 80, screenWidth, 80, HEADER_COLOR);
        DrawText("F5: Refresh   |   Delete: End Task   |   E/D: Enable/Disable Startup", 
                15, screenHeight - 35, 14, SUBTLE_TEXT_COLOR);
        
        EndDrawing();
    }
    
    CleanupProcessList();
    CleanupStartupList();
    CleanupAppHistory();
    CloseWindow();
    
    return 0;
}