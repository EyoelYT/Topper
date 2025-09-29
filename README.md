# TOPPER

topper is a tiny C++ console application for Windows that lets you search, filter and interactively select any top-level window by title, then toggle its “always-on-top” (`WS_EX_TOPMOST`) state. It’s perfect for anyone who wants a quick keyboard-driven way to pin/unpin windows without touching the mouse or hunting through window menus.

## Requirements
This program is intended to work only on the Windows Operating System.

## Build
Use `Developer Command Prompt for VS 22` console to build the project, which can be installed using the [Visual Studio Installer](https://visualstudio.microsoft.com/downloads).

To build, use this command from the root directory:
``` shell
.\build.bat
```

To build and run, use this command from the root directory:
``` shell
.\build.bat && .\topper.exe
```

## Usage
Copy the executable, pin it to Start, or add it into your `$PATH` to access it from anywhere.

### Toggle Windows on Top (`--twot`)
`--twot` stands for "toggle window on top". Just `topper` would also invoke the same command.

Toggle windows on top in this way:
``` shell
topper --twot
```

- Use the `up` and `down` arrow keys to choose between windows.
- Enter characters to narrow down windows.
- Press `Enter` to choose a window to toggle.
- Press `Escape` to exit without doing anything.

### Help menu (`--help`)
```shell
topper --help
```

## Acknowledgements
This project was inspired by [PowerToys' Window on Top Feature](https://learn.microsoft.com/en-us/windows/powertoys/always-on-top), and is meant to be a very lightweight TUI version.
