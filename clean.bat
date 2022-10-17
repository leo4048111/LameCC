@echo off
set BUILD_PATH="%cd%\build\"

if exist %BUILD_PATH% (
    rd /s /q %BUILD_PATH%
)