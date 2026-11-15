@echo off
cd /d "%~dp0"
rem Requires CMake, a C++17 compiler and libsodium development files.
cmake -S . -B build
if errorlevel 1 exit /b 1
cmake --build build --config Release
