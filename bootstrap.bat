@echo off
start "" cmd.exe /c "vscmd19_64.bat & cl /nologo /Z7 /MTd /Fdbuild /Febuild /DWIN src/main_build.cpp /link /INCREMENTAL:NO & del main_build.obj & echo Created build.exe! & pause"
