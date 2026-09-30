// Держит левый Shift только в окне игры (LWJGL / GLFW30) того процесса, куда инжектнута DLL.
#include <windows.h>

static volatile bool g_run = true;
static HANDLE g_thread = nullptr;
static HWND g_lastWnd = nullptr;

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

static DWORD WINAPI HoldShift(LPVOID)
{
    bool wasFocused = false;
    while (g_run) {
        HWND h = FindGameWindow();
        if (h) {
            bool focused = (GetForegroundWindow() == h);
            // жмём Shift при новом окне или когда окно снова получило фокус
            if (h != g_lastWnd || (focused && !wasFocused))
                PostMessageW(h, WM_KEYDOWN, VK_SHIFT, DownParam());
            g_lastWnd = h;
            wasFocused = focused;
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
