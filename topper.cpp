#define NOMINMAX // do not use min max definitions of windows.h
#include <algorithm>
#include <cctype>
#include <conio.h>
#include <cstdint>
#include <dwmapi.h>
#include <optional>
#include <stdio.h>
#include <string>
#include <vector>
#include <windows.h>

struct WindowInfo {
    HWND hWnd;
    std::string title;
};

std::string IsTopMost(HWND hWnd) {
    LONG IsTopMost = (GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
    if (IsTopMost) { return "TOPMOST"; } else { return "NOT TOPMOST"; }
}

// return a copy of the string but lower-cased
std::string StringToLower(const std::string& originalString) {
    std::string lowerCaseString = originalString;

    // Apply std::tolower to each character in the copy
    std::transform(lowerCaseString.begin(), lowerCaseString.end(),
                   lowerCaseString.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    return lowerCaseString;
}

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

    std::string title(length + 1, '\0');
    GetWindowText(hwnd, &title[0], length + 1);
    // Remove trailing null
    title.resize(length);

    WindowInfo windowInfo;
    windowInfo.hWnd = hwnd;
    windowInfo.title = title;

    windows->push_back(windowInfo);
    return TRUE;
}

COORD GetCursorCoords() {

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    return csbi.dwCursorPosition;
}

void DrawMenu(const std::vector<WindowInfo>& wins, int selected, COORD startPos, BOOL makeSpace, uint32_t numLines) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    DWORD written;
    // For each window to be printed as a choice, clear out some space
    if (makeSpace) {
        COORD linePos = { 0, SHORT(startPos.Y) }; // start from the line under the current line
        FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X * numLines, linePos, &written);
    }

    // Print the choices
    for (uint32_t i = 0; i < wins.size() ; i++) {
        COORD linePos = { 0, SHORT(startPos.Y + i) };
        SetConsoleCursorPosition(hConsole, linePos);

        // Highlight if selected
        if (i == selected)
            printf("> %11s : %s  ", IsTopMost(wins[i].hWnd).c_str(), wins[i].title.c_str());
        else
            printf("  %11s : %s  ", IsTopMost(wins[i].hWnd).c_str(), wins[i].title.c_str());

        printf("\n");
    }
    SetConsoleCursorPosition(hConsole, { csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y }); // reset cursor position to the input buffer position
}

std::vector<WindowInfo> fuzzySearch(const std::string query, const std::vector<WindowInfo>& windows) {
    std::vector<WindowInfo> results;
    for (const WindowInfo window : windows) {
        if (StringToLower(window.title).find(StringToLower(query)) != std::string::npos) {
            results.push_back(window);
        }
    }
    return results;
}

std::optional<WindowInfo> PickWindow(const std::vector<WindowInfo>& windows) { // TODO: this should return a WindowInfo
    if (windows.empty()) return std::nullopt;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    COORD originalPos = GetCursorCoords();
    COORD menuStartPos;

    int requiredLines = windows.size();
    int availableLines = csbi.dwSize.Y - originalPos.Y - 1; // -1 bc of extra newline

    if (availableLines < requiredLines) {
        // make space with empty lines
        int linesToScroll = requiredLines - availableLines;
        for (int i = 0; i < linesToScroll; i++) {
            printf("\n");
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

    const int charMaxLength = 512;
    int charBufferPtr = 0;
    char charBuffer[charMaxLength];

    std::string initialString("");
    std::vector<WindowInfo> fuzziedWindows = fuzzySearch(initialString, windows);

    int selected = 0;
    uint32_t totalWindowsNumber = windows.size();
    DrawMenu(fuzziedWindows, selected, menuStartPos, true, totalWindowsNumber);

    while (1) {
        int c = _getch();
        if (c == 0 || c == 0xE0) {
            c = _getch();
            switch (c) {
            case 72: // up arrow
                selected = (selected + fuzziedWindows.size() - 1) % fuzziedWindows.size();
                DrawMenu(fuzziedWindows, selected, menuStartPos, false, totalWindowsNumber);
                break;
            case 80: // down arrow
                selected = (selected + 1) % fuzziedWindows.size();
                DrawMenu(fuzziedWindows, selected, menuStartPos, false, totalWindowsNumber);
                break;
            }
        } else if (c == 13) { // enter
            if (fuzziedWindows.size() <= 0) {
                return std::nullopt;
            }
            return fuzziedWindows[std::min(static_cast<size_t>(selected), fuzziedWindows.size())];
        } else if (c == 27) { // esc
            printf("\nExit\n");
            return std::nullopt;
        } else if (c >= 32 && c <= 126) { // printable ascii chars
            char inputCharacter = (char)c;
            if (charBufferPtr + 1 > charMaxLength) {
                abort();
            }
            charBuffer[charBufferPtr] = inputCharacter;
            charBufferPtr += 1;
            if (charBufferPtr >= charMaxLength) {
                abort();
            }
            charBuffer[charBufferPtr] = '\0';

            printf("%c", inputCharacter);

            selected = 0;

            std::string charBufferAsStr(charBuffer);
            fuzziedWindows = fuzzySearch(charBufferAsStr, windows);
            DrawMenu(fuzziedWindows, selected, menuStartPos, true, totalWindowsNumber);
        } else if (c == 8) { // backspace
            charBufferPtr -= 1;
            if (charBufferPtr < 0) {
                charBufferPtr = 0;
            }
            charBuffer[charBufferPtr] = '\0';

            printf("\b \b"); // delete previous character

            selected = 0;
            std::string charBufferAsStr(charBuffer);
            fuzziedWindows = fuzzySearch(charBufferAsStr, windows);
            DrawMenu(fuzziedWindows, selected, menuStartPos, true, totalWindowsNumber);
        }
    }
}

int main(int argc, char* argv[]) {

    std::string twotCommand = "--twot";
    std::string helpCommand = "--help";

    printf("----------------------------------------------------------------------\n");
    if (argc < 2 || argv[1] == helpCommand) {
        // toggle window on top help
        printf("Usage:\n  %s %s\n", argv[0], twotCommand.c_str());
        return 1;
    }

    std::string command = argv[1];

    if (command == twotCommand) {
        std::vector<WindowInfo> windows;
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));

        std::optional<WindowInfo> selectedWindowInfoOptional = PickWindow(windows);
        if (selectedWindowInfoOptional) {
            WindowInfo selectedWindowInfo = selectedWindowInfoOptional.value();
            printf("\nSelected window: %s; WINDOW: \n", selectedWindowInfo.title.c_str());

            LONG_PTR windowExStyle = GetWindowLongPtr(selectedWindowInfo.hWnd, GWL_EXSTYLE);
            BOOL wasTopMost = (windowExStyle & WS_EX_TOPMOST) != 0;

            HWND insertAfter = wasTopMost ? HWND_NOTOPMOST : HWND_TOPMOST;
            BOOL success = SetWindowPos(selectedWindowInfo.hWnd, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

            if (success && wasTopMost) {
                printf("%s is now NOT TOPMOST.\n", selectedWindowInfo.title.c_str());
            } else if (success && !wasTopMost) {
                printf("%s is now TOPMOST.\n", selectedWindowInfo.title.c_str());
            }
        } else {
            printf("\nNo selected window\n");
            return 0;
        }
    }

    return 0;
}
