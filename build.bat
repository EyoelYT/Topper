rem call "c:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvarsall.bat" x64
cl topper.cpp -Zi User32.lib Dwmapi.lib Kernel32.lib /EHsc
