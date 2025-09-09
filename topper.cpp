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

COORD GetCursorCoords() {

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    return csbi.dwCursorPosition;
}

void DrawMenu(const std::vector<WindowInfo>&wins, int selected, COORD startPos, BOOL makeSpace) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    // Remember old attributes
    WORD oldAttr = csbi.wAttributes;

    DWORD written;
    // For each window to be printed as a choice, clear out some space
    if (makeSpace) {
        for (int i = 0; i < (int)wins.size(); i++) {
            COORD linePos = { 0, SHORT(startPos.Y + i) }; // start from the line under the current line
            FillConsoleOutputCharacterW(hConsole, L' ', csbi.dwSize.X, linePos, &written);
        }
    }

    // Print the choices
    for (int i = 0; i < (int)wins.size() ; i++) {
        COORD linePos;
        linePos = { 0, SHORT(startPos.Y + i) };
        SetConsoleCursorPosition(hConsole, linePos);

        // Highlight if selected
        if (i == selected) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_BLUE);
        }

        // Print a choice
        wprintf(L" %2d: %s  ", i+1, wins[i].title.c_str());

        // Restore text attributes
        SetConsoleTextAttribute(hConsole, oldAttr);
        wprintf(L"\n");
    }
    wprintf(L"\n"); // add a newline at end of menu
}

int PickWindow(const std::vector<WindowInfo>&wins) {
    if (wins.empty()) return 1;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    COORD originalPos = GetCursorCoords();
    COORD menuStartPos;

    int requiredLines = wins.size();
    int availableLines = csbi.dwSize.Y - originalPos.Y - 1;

    if (availableLines < requiredLines) {
        // make space with empty lines
        int linesToScroll = requiredLines - availableLines + 1; // +1 bc we add a new line as part of the menu
        for (int i = 0; i < linesToScroll; i++) {
            wprintf(L"\n");
        }

        // update cursor position (move backwards (go -ve))
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        menuStartPos.X = 0;
        menuStartPos.Y = csbi.dwCursorPosition.Y - linesToScroll;

        // no negative lines
        if (menuStartPos.Y < 0) {
            menuStartPos.Y = 0;
        }
    } else {
        menuStartPos.X = 0;
        menuStartPos.Y = originalPos.Y;
    }

    int selected = 0;
    DrawMenu(wins, selected, menuStartPos, true);

    while (1) {
        int c = _getch();
        if (c == 0 || c == 0xE0) {
            c = _getch();
            switch (c) {
            case 72: // up
                selected = (selected + wins.size() - 1) % wins.size();
                DrawMenu(wins, selected, menuStartPos, false);
                break;
            case 80: // down
                selected = (selected + 1) % wins.size();
                DrawMenu(wins, selected, menuStartPos, false);
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

    std::wstring twotCommand = L"--toggle-wot";

    wprintf(L"----------------------------------------------------------------------\n");
    if (argc < 2 || argv[1] == L"--help") {
        // toggle window on top help
        wprintf(L"Usage:\n  %ls %ls\n", argv[0], twotCommand.c_str()); // toggle window on top help
        return 1;
    }

    std::wstring command = argv[1];

    if (command == twotCommand) {
        std::vector<WindowInfo> windows;
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));
        int count = 0;

        int idx = PickWindow(windows);
        if (idx < 0) {
            return 0;
        }
    }

    return 0;
}
