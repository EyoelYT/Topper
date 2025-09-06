#include <conio.h>
#include <dwmapi.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <windows.h>

struct WindowInfo {
    HWND hWnd;
    std::wstring title;
};

BOOL IsAltTabWindow(HWND hwnd) {
    HWND hwndWalk = GetAncestor(hwnd, GA_ROOTOWNER);

    // Reject the invisible windows
    if (!IsWindowVisible(hwnd))
        return FALSE;

    // check for "cloaked" UWP windows (e.g. touch keyboard)
    BOOL cloaked = FALSE;
    DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (cloaked)
      return FALSE;

    // Find true root owner of this window
    while (1) {
        HWND hwndTry = GetLastActivePopup(hwndWalk);
        if (hwndTry == hwndWalk)
            break;
        if (IsWindowVisible(hwndTry)) {
            hwndWalk = hwndTry;
            break;
        }
        hwndWalk = hwndTry;
    }

    if (GetWindow(hwnd, GW_OWNER))
        return TRUE;

    // Reject tool windows unless they have WS_EX_APPWINDOW
    LONG_PTR extendedStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if ((extendedStyle & WS_EX_TOOLWINDOW) && !(extendedStyle & WS_EX_APPWINDOW))
        return FALSE;

    return hwndWalk == hwnd;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    if (!IsAltTabWindow(hwnd))
        return TRUE;

    int length = GetWindowTextLength(hwnd);
    if (length == 0)
        return TRUE;

    std::vector<WindowInfo>* windows = reinterpret_cast<std::vector<WindowInfo>*>(lParam);

    std::wstring title(length + 1, L'\0');
    GetWindowTextW(hwnd, &title[0], length + 1);
    // Remove trailing null
    title.resize(length);

    WindowInfo info;
    info.hWnd = hwnd;
    info.title = title;

    windows->push_back(info);
    return TRUE;
}

COORD GetCurrentCursorCoords() {

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    return { csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y };
}

void DrawMenu(const std::vector<WindowInfo>&wins, int selected, COORD cursorPos) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    // remember old
    WORD oldAttr = csbi.wAttributes;
    DWORD written;
    COORD home = { cursorPos.X, cursorPos.Y };

    // if (csbi.dwSize.Y - (cursorPos.Y + 1) < wins.size() + 1) {
    //     home = { cursorPos.X, csbi.dwSize.Y - static_cast<SHORT>(wins.size() + 1) };
    //     // Scroll down by (wins.size() + 1) amount so that the tui doesn't screw with the previous console output
    //     for (int i = 0; i < wins.size() + 2; i++) {
    //         wprintf(L"\n");
    //     }
    // } else {
    //     home = { cursorPos.X, cursorPos.Y + 1};
    // }

    SetConsoleCursorPosition(hConsole, home);
    FillConsoleOutputCharacter(hConsole, ' ', (csbi.dwSize.X) * (csbi.dwSize.Y - (wins.size() + 1)), home, &written);

    // TODO: what I really need: refresh the current interactive part of the terminal
    for (int i = 0; i < (int)wins.size(); i++) {
        if (i == selected) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_BLUE);
        }
        wprintf(L" %2d: %s\n", i+1, wins[i].title.c_str());
        SetConsoleTextAttribute(hConsole, oldAttr);
    }

}

int PickWindow(const std::vector<WindowInfo>&wins) {
    if (wins.empty()) return 1;

    int selected = 0;
    COORD cursorCoord = GetCurrentCursorCoords();

    // Modify initial cursor position
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    COORD targetCoord;
    if (csbi.dwSize.Y - (csbi.dwCursorPosition.Y + 1) < wins.size() + 1) {
        targetCoord.Y = csbi.dwSize.Y - static_cast<SHORT>(wins.size() + 1);
        // TODO: Scroll down by (wins.size() + 1) amount so that the tui doesn't screw with the previous console output
        // for (int i = 0; i < wins.size() + 1; i++) {
        //     wprintf(L"\n");
        // }
    } else {
        targetCoord.Y = csbi.dwCursorPosition.Y + 1;
    }

    DrawMenu(wins, selected, targetCoord);

    while (1) {
        int c = _getch();
        if (c == 0 || c == 0xE0) {
            c = _getch();
            switch (c) {
            case 72: // up
                selected = (selected + wins.size() - 1) % wins.size();
                DrawMenu(wins, selected, targetCoord);
                break;
            case 80: // down
                selected = (selected + 1) % wins.size();
                DrawMenu(wins, selected, targetCoord);
                break;
            }
        } else if (c == 13) { // enter
            wprintf(L"Selected %d\n", selected + 1);
            return selected;
        } else if (c == 27) { // esc
            wprintf(L"No selection\n");
            return -1;
        }
    }
}

int wmain(int argc, wchar_t* argv[]) {

    wprintf(L"----------------------------------------------------------------------\n"); // toggle window on top help
    if (argc < 2 || argv[1] == L"--help") {
        wprintf(L"Usage:\n  %ls --toggle-wot\n", argv[0]); // toggle window on top help
        return 1;
    }

    std::wstring command = argv[1];

    if (command == L"--toggle-wot") {
        std::vector<WindowInfo> windows;
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));
        int count = 0;
        for (auto& w : windows) {
            count++;
            wprintf(L"%d -> Window: %ls\n", count, w.title.c_str()); // add " | ONTOP=T/F" indication to the right of the string
        }

        // tui logic here??
        int idx = PickWindow(windows);
        if (idx < 0) {
            return 0;
        }
    }

    return 0;
}
