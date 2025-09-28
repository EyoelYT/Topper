## Requirements
This program is intended to work only on the Windows Operating System.

## Build
Use `Developer Command Prompt for VS 22` to build the project, which can be installed using the [Visual Studio Installer](https://visualstudio.microsoft.com/downloads).

To build, use this command from the root directory:
``` shell
.\build.bat
```

To build and run, use this command from the root directory:
``` shell
.\build.bat && .\topper.exe
```

## Usage
Toggle windows on top in this way:
``` shell
topper --twot
```
"--twot" stands for "toggle window on top".
Just "topper" would also invoke the same command.

Help menu:
```shell
topper --help
```

Copy the executable, pin it to Start, or add it into your `$PATH` to access it from anywhere.

## Acknowledgements
This project was inspired by [PowerToys' Window on Top Feature](https://github.com/microsoft/PowerToys)
