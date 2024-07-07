#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <windows.h>
#include <psapi.h>
#include <vector>
#include <tlhelp32.h>

constexpr COLORREF kTargetColor = RGB(43, 137, 254);
constexpr DWORD kHotkeyId = 1;

std::atomic<bool> g_isActive(false);
std::atomic<bool> g_autoTapXActive(false);
std::atomic<bool> g_moveCursor(false);
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

void ReleaseHdc(HDC hdc) {
	if (hdc) {
		ReleaseDC(nullptr, hdc);
	}
}

void DeleteGdiObject(HGDIOBJ obj) {
	if (obj) {
		DeleteObject(obj);
	}
}

using UniqueHdc = std::unique_ptr<std::remove_pointer<HDC>::type, 
decltype(&ReleaseHdc)>;
using UniqueBitmap = std::unique_ptr<std::remove_pointer<HBITMAP>::type, 
decltype(&DeleteGdiObject)>;

void AutoEdge() {
	UniqueHdc hdcScreen(GetDC(nullptr), ReleaseHdc);
	if (!hdcScreen) {
		return;
	}

	UniqueHdc hdcMem(CreateCompatibleDC(hdcScreen.get()), [](HDC hdc) { DeleteDC(hdc); });
	if (!hdcMem) {
		return;
	}

	int height = g_yEnd - g_yStart;
	UniqueBitmap hBitmap(CreateCompatibleBitmap(hdcScreen.get(), 1, height), DeleteGdiObject);
	if (!hBitmap) {
		return;
	}

	SelectObject(hdcMem.get(), hBitmap.get());

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = 1;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;

	const int stride = ((24 * 1 + 31) / 32) * 4;
	std::vector<BYTE> buffer(stride * height);

	const int cachedXCoord = g_xCoord;
	const int cachedYStart = g_yStart;

	const BYTE targetRed = GetRValue(kTargetColor);
	const BYTE targetGreen = GetGValue(kTargetColor);
	const BYTE targetBlue = GetBValue(kTargetColor);

	while (g_programRunning.load()) {
		if (g_isActive.load() && IsRobloxFocused()) {
			BitBlt(hdcMem.get(), 0, 0, 1, height, hdcScreen.get(), cachedXCoord, cachedYStart, SRCCOPY);

			if (GetDIBits(hdcMem.get(), hBitmap.get(), 0, height, buffer.data(), &bmi, DIB_RGB_COLORS)) {
				for (int y = 0; y < height; ++y) {
					int index = y * stride;
					if (buffer[index + 2] == targetRed &&
						buffer[index + 1] == targetGreen &&
						buffer[index + 0] == targetBlue) {
						if (g_moveCursor.load()) {
							SetCursorPos(cachedXCoord, cachedYStart + y);
						}
						Click();
						break;
					}
				}
			}
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}
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

void ToggleMoveCursor() {
	g_moveCursor = !g_moveCursor;
	LogMessage(g_moveCursor ? "move cursor mode enabled" : "move cursor mode disabled");
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
			LogMessage("movecursor - toggle moving cursor to the pixel position");
			LogMessage("autoedge - toggle auto-edging");
		} else if (command == "exit") {
			g_autoTapXActive = false;
			g_isActive = false;
			g_programRunning = false;
		} else if (command == "autotapx") {
			ToggleAutoTapX();
		} else if (command == "autoedge") {
			ToggleAutoEdge();
		} else if (command == "movecursor") {
			ToggleMoveCursor();
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

bool isRobloxOn() {
	HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
	CloseHandle(hProcessSnap);
        return false;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap); // clean the snapshot object
        return false;
    }

    do {
        if (strcmp(pe32.szExeFile, "RobloxPlayerBeta.exe") == 0) {
            CloseHandle(hProcessSnap);
            return true;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return false;
}

int main() {
	SetConsoleTitle(TEXT("auto-edger"));
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	if (!SetProcessDPIAware()) {
		LogMessage("warning: failed to set process dpi awareness");
	}

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	AccountForDifferentRes(screenWidth, screenHeight);

	if (!isRobloxOn()) {
		LogMessage("warning: roblox is not running\nturn on roblox, dummy");

		while (!isRobloxOn()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}

	if (!RegisterHotKey(nullptr, kHotkeyId, 0, VK_F8)) {
		LogMessage("failed to register hotkey for f8. use 'autoedge' command to toggle");
	} else {
		LogMessage("press f8 to outedge everyone");
	}

	LogMessage("type 'help' for a list of commands");

	std::thread autoEdgeThread(AutoEdge);
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
	if (autoEdgeThread.joinable()) {
		autoEdgeThread.join();
	}

	return 0;
}
