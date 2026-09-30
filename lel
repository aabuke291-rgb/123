// Минимальная DLL: после инжекта просто зажимает левый Shift и держит его.
// При выгрузке DLL отпускает клавишу.
#include <windows.h>

static volatile bool g_run = true;
static HANDLE g_thread = nullptr;

static void SendShift(bool down)
{
    INPUT in = {};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = VK_LSHIFT;
    in.ki.wScan = (WORD)MapVirtualKeyW(VK_LSHIFT, MAPVK_VK_TO_VSC);
    in.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
    SendInput(1, &in, sizeof(INPUT));
}

static DWORD WINAPI HoldShift(LPVOID)
{
    SendShift(true);
    // Раз в 50 мс проверяем, что Shift всё ещё зажат, и при необходимости зажимаем снова
    while (g_run) {
        if (!(GetAsyncKeyState(VK_LSHIFT) & 0x8000))
            SendShift(true);
        Sleep(50);
    }
    SendShift(false);
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
