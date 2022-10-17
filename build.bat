@echo off
set BUILD_PATH="%cd%\build\"

if not exist %BUILD_PATH% (
    mkdir %BUILD_PATH%
)

cd /d %BUILD_PATH%

cmake .. -G "MinGW Makefiles"
make all