@echo off
pushd .
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\vsdevcmd.bat" ^
-arch=amd64 ^
-host_arch=amd64 ^
-no_logo
popd
