@echo off
call cmake -B Build/Projects -G "Visual Studio 17 2022"
popd
exit 0
