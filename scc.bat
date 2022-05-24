@echo off
pushd "c:/doc/dev/cpp/proj/main"
svn add . --force
svn ci -m "auto"
svn up
popd
