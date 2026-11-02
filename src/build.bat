@echo off
rem Builds Block Out III with the Visual Studio 2022 C++ tools (x64).
rem   build.bat            Release build
rem   build.bat test       Release build + regression test
rem   build.bat debug      Debug build
rem   build.bat clean      delete the build folder first (combinable: build.bat clean test)
setlocal
for %%i in ("%~dp0..") do set "ROOT=%%~fi"
set CONFIG=Release
set TEST=
set CLEAN=
:args
if "%~1"=="" goto argsdone
if /i "%~1"=="debug" set CONFIG=Debug
if /i "%~1"=="test" set TEST=1
if /i "%~1"=="clean" set CLEAN=1
shift
goto args
:argsdone
set "OUT=%ROOT%\build\%CONFIG%"
if defined CLEAN if exist "%OUT%" rmdir /s /q "%OUT%"

rem Import the x64 developer environment unless it is already set
if defined VCToolsInstallDir goto envdone
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set VS=
if exist "%VSWHERE%" for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -property installationPath`) do set "VS=%%i"
if not defined VS set "VS=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
if not exist "%VS%\VC\Auxiliary\Build\vcvars64.bat" (echo Visual Studio 2022 C++ tools not found & exit /b 1)
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
:envdone

cmake -S "%ROOT%" -B "%OUT%" -G Ninja -DCMAKE_BUILD_TYPE=%CONFIG% -DCMAKE_CXX_COMPILER=cl >nul || (echo CMake configure failed & exit /b 1)
cmake --build "%OUT%" || (echo Build failed & exit /b 1)

if not defined TEST goto done
pushd "%OUT%"
"%OUT%\regress.exe"
set RC=%ERRORLEVEL%
popd
if not "%RC%"=="0" (echo Regression test failed & exit /b 1)
:done
echo Built %OUT%\BlockOut3.exe
