@echo off
REM Build script for AirPlay VST3 Plugin (Windows)

echo ====================================
echo FreeCaster Build Script
echo ====================================
echo.

REM Build configuration
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Release

echo Build configuration: %CONFIG%
echo.

REM Check for Visual Studio
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake and add it to your PATH
    exit /b 1
)

REM Create build directory
if exist build (
    echo Build directory exists. Cleaning...
    rmdir /s /q build
)

mkdir build
cd build

REM Run CMake
echo Running CMake...
cmake .. -G "Visual Studio 17 2022"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed
    echo If you have a different Visual Studio version, edit build.bat
    echo and change the generator name
    exit /b 1
)

REM Build
echo.
echo Building plugin...
cmake --build . --config %CONFIG%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo ====================================
echo Build complete!
echo ====================================
echo.
echo Plugin location:
echo   VST3: AirPlayPlugin_artefacts\%CONFIG%\VST3\FreeCaster.vst3
echo   Standalone: AirPlayPlugin_artefacts\%CONFIG%\Standalone\FreeCaster.exe
echo.
echo To install the plugin, see BUILD.md

cd ..
