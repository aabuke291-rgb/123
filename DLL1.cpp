// Держит левый Shift, пока активно окно процесса и функция включена кнопкой 'Z'.
// Повторное нажатие 'Z' отключает удерживание.
#include <windows.h>

static volatile bool g_run = true;
static HANDLE g_thread = nullptr;
static bool g_held = false;
static bool g_enabled = false;

static void SendShift(bool down)
{
    INPUT in = {};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = VK_LSHIFT;
    in.ki.wScan = (WORD)MapVirtualKeyW(VK_LSHIFT, MAPVK_VK_TO_VSC);
    in.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
    SendInput(1, &in, sizeof(INPUT));
}

static bool GameIsActive()
{
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    return pid == GetCurrentProcessId();
}

static DWORD WINAPI HoldShift(LPVOID)
{
    bool zWasDown = false;

    while (g_run) {
        bool active = GameIsActive();

        // Переключение состояния по нажатию 'Z' (переключатель)
        bool zIsDown = (GetAsyncKeyState('Z') & 0x8000) != 0;
        if (zIsDown && !zWasDown && active) {
            g_enabled = !g_enabled;
        }
        zWasDown = zIsDown;

        // Зажатие работает только при активном окне и включенном режиме
        if (active && g_enabled) {
            if (!g_held || !(GetAsyncKeyState(VK_LSHIFT) & 0x8000)) {
                SendShift(true);
                g_held = true;
            }
        } else if (g_held) {
            SendShift(false);
            g_held = false;
        }

        Sleep(20);
    }

    if (g_held) SendShift(false);
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
        if (g_thread) {
            WaitForSingleObject(g_thread, 500);
            CloseHandle(g_thread);
        }
        break;
    }
    return TRUE;
}
