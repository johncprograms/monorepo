@echo off
pushd "c:/doc/dev/cpp/proj/main"
call svn diff --diff-cmd c:/doc/bin/svn_to_winmerge.bat
popd