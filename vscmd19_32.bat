@echo off
pushd .
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\vsdevcmd.bat" ^
-arch=x86 ^
-host_arch=amd64 ^
-no_logo
popd
