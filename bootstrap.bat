@echo off
start "" cmd.exe /c "vscmd22_64.bat & cl /nologo /Z7 /MTd /Fdbuild /Febuild /DWIN /std:c++20 src/main_build.cpp /link /INCREMENTAL:NO & del main_build.obj & echo Created build.exe! & pause"
