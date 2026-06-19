@echo off
setlocal enabledelayedexpansion
REM =============================================================
REM build_deps.bat - 构建非 CMake 依赖 (OpenSSL, Python 2.7)
REM
REM 前提条件:
REM   1. Visual Studio 2015~2022 (在 Developer Command Prompt 中运行)
REM   2. Strawberry Perl (https://strawberryperl.com/) 或 ActivePerl
REM   3. NASM (可选，加 no-asm 可跳过)
REM
REM 用法:
REM   build_deps.bat [x64|x86] [Release|Debug]
REM =============================================================

set SCRIPT_DIR=%~dp0
set ARCH=%~1
set CONFIG=%~2
if "%ARCH%"=="" set ARCH=x64
if "%CONFIG%"=="" set CONFIG=Release

set DEPS_DIR=%SCRIPT_DIR%..\deps\%ARCH%
set OPENSSL_SRC=%SCRIPT_DIR%third_party\openssl
set PYTHON_SRC=%SCRIPT_DIR%Python-2.7.18

REM 检测编译器环境
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: cl.exe not found.
    echo Please run this script from a Visual Studio Developer Command Prompt.
    echo Example: "x64 Native Tools Command Prompt for VS 2022"
    exit /b 1
)

REM 检测 Perl
where perl >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Perl not found.
    echo Install Strawberry Perl: https://strawberryperl.com/
    exit /b 1
)

echo ================================================================
echo Building dependencies for %ARCH% %CONFIG%
echo Output: %DEPS_DIR%
echo ================================================================

if not exist "%DEPS_DIR%" mkdir "%DEPS_DIR%"

REM =============================================================
REM 1. OpenSSL
REM =============================================================
echo.
echo === [1/2] Building OpenSSL ===
echo.

if exist "%DEPS_DIR%\openssl\lib\libcrypto.lib" (
    echo OpenSSL already built, skipping. Delete %DEPS_DIR%\openssl to rebuild.
    goto :build_python
)

cd /d "%OPENSSL_SRC%"

if "%ARCH%"=="x64" (
    set OPENSSL_TARGET=VC-WIN64A
) else (
    set OPENSSL_TARGET=VC-WIN32
)

REM Configure OpenSSL
perl Configure %OPENSSL_TARGET% --prefix="%DEPS_DIR%\openssl" no-shared no-tests no-asm
if %errorlevel% neq 0 (
    echo ERROR: OpenSSL Configure failed.
    exit /b 1
)

REM Build
nmake
if %errorlevel% neq 0 (
    echo ERROR: OpenSSL build failed.
    exit /b 1
)

REM Install (只安装库和头文件，不安装文档)
nmake install_sw
if %errorlevel% neq 0 (
    echo ERROR: OpenSSL install failed.
    exit /b 1
)

REM 清理源码目录的构建产物
nmake clean

echo OpenSSL built successfully.

:build_python
REM =============================================================
REM 2. Python 2.7
REM =============================================================
echo.
echo === [2/2] Building Python 2.7 ===
echo.

if exist "%DEPS_DIR%\python27\lib\python27.lib" (
    echo Python 2.7 already built, skipping. Delete %DEPS_DIR%\python27 to rebuild.
    goto :done
)

if not exist "%PYTHON_SRC%" (
    echo ERROR: Python 2.7 source not found at %PYTHON_SRC%
    exit /b 1
)

cd /d "%PYTHON_SRC%\PCbuild"

REM 用 MSBuild 编译 pythoncore (静态库)
if "%ARCH%"=="x64" (
    set PLATFORM=x64
) else (
    set PLATFORM=Win32
)

msbuild pythoncore.vcxproj /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /m
if %errorlevel% neq 0 (
    echo ERROR: Python 2.7 pythoncore build failed.
    exit /b 1
)

REM 编译扩展模块
msbuild _socket.vcxproj /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /m
msbuild select.vcxproj /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /m 2>nul
msbuild _ctypes.vcxproj /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /m

REM 安装到 deps 目录
if not exist "%DEPS_DIR%\python27\lib" mkdir "%DEPS_DIR%\python27\lib"
if not exist "%DEPS_DIR%\python27\include" mkdir "%DEPS_DIR%\python27\include"

REM 复制库文件
if "%ARCH%"=="x64" (
    set PYOUT=%PYTHON_SRC%\PCbuild\amd64
) else (
    set PYOUT=%PYTHON_SRC%\PCbuild
)

copy "%PYOUT%\python27.lib" "%DEPS_DIR%\python27\lib\" 2>nul
copy "%PYOUT%\python27_d.lib" "%DEPS_DIR%\python27\lib\" 2>nul
copy "%PYOUT%\_socket.lib" "%DEPS_DIR%\python27\lib\" 2>nul
copy "%PYOUT%\select.lib" "%DEPS_DIR%\python27\lib\" 2>nul
copy "%PYOUT%\_ctypes.lib" "%DEPS_DIR%\python27\lib\" 2>nul

REM 复制头文件
xcopy /E /Y /Q "%PYTHON_SRC%\Include\*" "%DEPS_DIR%\python27\include\"
copy "%PYTHON_SRC%\PC\pyconfig.h" "%DEPS_DIR%\python27\include\"

echo Python 2.7 built successfully.

:done
echo.
echo ================================================================
echo All dependencies built successfully!
echo.
echo Now configure your project:
echo   cmake -B build-win -G "Visual Studio 17 2022" -A %ARCH%
echo   cmake --build build-win --config %CONFIG%
echo ================================================================

cd /d "%SCRIPT_DIR%"
endlocal
