// ДИАГНОСТИЧЕСКАЯ версия: показывает, что DLL загрузилась и нашла ли она окно игры.
#include <windows.h>

static volatile bool g_run = true;
static HANDLE g_thread = nullptr;
static HWND g_lastWnd = nullptr;
static char g_foundClass[64] = {};

static const UINT SC_LSHIFT = 0x2A;

static LPARAM DownParam() { return (LPARAM)(1u | (SC_LSHIFT << 16)); }
static LPARAM UpParam()   { return (LPARAM)(1u | (SC_LSHIFT << 16) | (1u << 30) | (1u << 31)); }

static BOOL CALLBACK EnumProc(HWND h, LPARAM lp)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid != GetCurrentProcessId() || !IsWindowVisible(h))
        return TRUE;

    char cls[64] = {};
    GetClassNameA(h, cls, sizeof(cls));
    if (lstrcmpA(cls, "LWJGL") == 0 || lstrcmpA(cls, "GLFW30") == 0) {
        lstrcpyA(g_foundClass, cls);
        *(HWND*)lp = h;
        return FALSE;
    }
    return TRUE;
}

static HWND FindGameWindow()
{
    HWND h = nullptr;
    EnumWindows(EnumProc, (LPARAM)&h);
    return h;
}

// Список всех окон процесса (если игровое не найдено)
static BOOL CALLBACK ListProc(HWND h, LPARAM lp)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid != GetCurrentProcessId())
        return TRUE;
    char cls[64] = {};
    GetClassNameA(h, cls, sizeof(cls));
    char* buf = (char*)lp;
    if (lstrlenA(buf) + lstrlenA(cls) + 2 < 900) {
        lstrcatA(buf, cls);
        lstrcatA(buf, "\n");
    }
    return TRUE;
}

static DWORD WINAPI HoldShift(LPVOID)
{
    MessageBoxA(nullptr, "DLL загружена", "Диагностика", MB_OK | MB_TOPMOST);

    bool reported = false;
    bool wasFocused = false;
    int tries = 0;
    while (g_run) {
        HWND h = FindGameWindow();
        if (h) {
            if (!reported) {
                reported = true;
                char msg[128] = "Окно игры найдено, класс: ";
                lstrcatA(msg, g_foundClass);
                MessageBoxA(nullptr, msg, "Диагностика", MB_OK | MB_TOPMOST);
            }
            bool focused = (GetForegroundWindow() == h);
            if (h != g_lastWnd || (focused && !wasFocused))
                PostMessageW(h, WM_KEYDOWN, VK_SHIFT, DownParam());
            g_lastWnd = h;
            wasFocused = focused;
        } else if (!reported && ++tries == 40) { // ~2 секунды без результата
            reported = true;
            char list[1024] = "Окно игры НЕ найдено. Окна процесса:\n";
            EnumWindows(ListProc, (LPARAM)list);
            MessageBoxA(nullptr, list, "Диагностика", MB_OK | MB_TOPMOST);
        }
        Sleep(50);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        g_thread = CreateThread(nullptr, 0, HoldShift, nullptr, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        g_run = false;
        if (g_lastWnd)
            PostMessageW(g_lastWnd, WM_KEYUP, VK_SHIFT, UpParam());
        if (g_thread)
            CloseHandle(g_thread);
        break;
    }
    return TRUE;
}
