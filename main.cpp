#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <windows.h>
#include <psapi.h>

constexpr COLORREF kTargetColor = RGB(43, 137, 254);
constexpr DWORD kHotkeyId = 1;

std::atomic<bool> g_isActive(false);
std::atomic<bool> g_autoTapXActive(false);
std::atomic<bool> g_programRunning(true);

std::thread autoTapXThread;

int g_xCoord;
int g_yStart;
int g_yEnd;

void LogMessage(const std::string& message) {
    std::cout << message << std::endl;
}

void UserInput() {
    std::cout << "> ";
    std::flush(std::cout);
}

void AccountForDifferentRes(int width, int height) {
    g_xCoord = static_cast<int>(width * 0.96);
    g_yStart = static_cast<int>(height * 0.37);
    g_yEnd = static_cast<int>(height * 0.72);
}

bool IsRobloxFocused() {
    HWND hwnd = GetForegroundWindow();
    if (hwnd == nullptr) {
        return false;
    }
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    WCHAR exeName[MAX_PATH];
    if (GetProcessImageFileNameW(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId), exeName, MAX_PATH) > 0) {
        return wcsstr(exeName, L"RobloxPlayerBeta.exe") != nullptr;
    }

    return false;
}

void Click() {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void ClickOnColorMatch(HDC hdc, int yStart, int yEnd, COLORREF targetColor) {
    for (int y = yStart; y < yEnd; ++y) {
        if (!g_isActive) {
            return;
        }

        COLORREF color = GetPixel(hdc, 0, y - yStart);
        if (color == targetColor) {
            Click();
            break;
        }
    }
}

void ScanForCouleur() {
    HDC hdcScreen = GetDC(nullptr);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, 1, g_yEnd - g_yStart);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 1;
    bmi.bmiHeader.biHeight = g_yEnd - g_yStart;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits;
    HBITMAP hDirectBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);

    while (g_programRunning) {
        if (g_isActive.load()) {
            HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hdcMem, hDirectBitmap));
            BitBlt(hdcMem, 0, 0, 1, g_yEnd - g_yStart, hdcScreen, g_xCoord, g_yStart, SRCCOPY);
            ClickOnColorMatch(hdcMem, g_yStart, g_yEnd, kTargetColor);
            SelectObject(hdcMem, hOldBitmap);
        }
    }

    DeleteObject(hDirectBitmap);
    DeleteDC(hdcMem);
    DeleteObject(hBitmap);
    ReleaseDC(nullptr, hdcScreen);
}

void AutoTapX() {
    const WORD xScanCode = MapVirtualKey(VkKeyScan('x'), MAPVK_VK_TO_VSC);

    while (g_autoTapXActive && g_programRunning) {
        if (IsRobloxFocused()) {
            INPUT ip;
            ip.type = INPUT_KEYBOARD;
            ip.ki.wScan = xScanCode;
            ip.ki.time = 0;
            ip.ki.dwExtraInfo = 0;
            ip.ki.wVk = 0;
            ip.ki.dwFlags = KEYEVENTF_SCANCODE;
            SendInput(1, &ip, sizeof(INPUT));

            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
            SendInput(1, &ip, sizeof(INPUT));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

void ToggleAutoTapX() {
    if (!g_autoTapXActive) {
        g_autoTapXActive = true;
        autoTapXThread = std::thread(AutoTapX);
        LogMessage("started auto tapping x");
    } else {
        g_autoTapXActive = false;
        if (autoTapXThread.joinable()) {
            autoTapXThread.join();
        }
        LogMessage("stopped auto tapping x");
    }
}

void ToggleAutoEdge() {
    g_isActive = !g_isActive;
    LogMessage(g_isActive ? "started auto-edging" : "stopped auto-edging");
}

void HandleCommands() {
    std::string command;
    UserInput();
    while (g_programRunning) {
        std::getline(std::cin, command);

        if (command == "help") {
            LogMessage("commands:");
            LogMessage("help - shows this message");
            LogMessage("exit - ropemaxx");
            LogMessage("autotapx - toggle auto tapping x every .3s (enters \'edge\' mode for you)");
            LogMessage("autoedge - toggle auto-edging");
        } else if (command == "exit") {
            g_autoTapXActive = false;
            g_isActive = false;
            g_programRunning = false;
        } else if (command == "autotapx") {
            ToggleAutoTapX();
        } else if (command == "autoedge") {
            ToggleAutoEdge();
        } else if (command == "author") {
            LogMessage("@z1xus");
        } else {
            LogMessage("i dont knowm this command yet!\ntype 'help' for a list of commands");
        }
        std::cout << std::endl;
        UserInput();
    }
    LogMessage("\nbye!");
}

int main() {
    SetConsoleTitle(TEXT("auto-edger"));

    if (!SetProcessDPIAware()) {
        LogMessage("warning: failed to set process dpi awareness");
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    AccountForDifferentRes(screenWidth, screenHeight);

    if (!RegisterHotKey(nullptr, kHotkeyId, 0, VK_F8)) {
        LogMessage("failed to register hotkey for f8. use 'autoedge' command to toggle");
    } else {
        LogMessage("press f8 to outedge everyone");
    }

    LogMessage("type 'help' for a list of commands");

    std::thread scanThread(ScanForCouleur);
    std::thread commandThread(HandleCommands);

    MSG msg = {};
     while (g_programRunning) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_HOTKEY && msg.wParam == kHotkeyId) {
                std::cout << std::endl;
                ToggleAutoEdge();
                std::cout << std::endl;
                UserInput();
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    UnregisterHotKey(nullptr, kHotkeyId);

    if (commandThread.joinable()) {
        commandThread.join();
    }
    if (scanThread.joinable()) {
        scanThread.join();
    }

    return 0;
}
