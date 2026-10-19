@echo off
rem === Better Snap Option - build (you run this; the compiler step is intentionally yours) ===========
rem Needs: llvm-mingw (clang++) and the ImGui + MinHook sources.
rem Defaults point at the copies unpacked in toolchain\ next to this file; override with env vars if elsewhere.
rem Compiler output goes to build\build.log (shown at the end on failure).  Clicking inside this window
rem only pauses the progress lines, not the compiler.
setlocal EnableDelayedExpansion
set "HERE=%~dp0"
if "%TC%"=="" set "TC=%HERE%toolchain\llvm-mingw-20261006-ucrt-x86_64\bin"
if "%IMGUI%"=="" set "IMGUI=%HERE%toolchain\imgui-1.92.9b"
if "%MINHOOK%"=="" set "MINHOOK=%HERE%toolchain\minhook-1.3.4"
set "CLANG=%TC%\clang++"
set "CC=%TC%\clang"
set "RC=%TC%\x86_64-w64-mingw32-windres"
set "OUT=%HERE%build"
set "LOG=%OUT%\build.log"
if not exist "%OUT%" mkdir "%OUT%"
echo Better Snap Option build  %date% %time%> "%LOG%"
echo TC=%TC%>> "%LOG%"
echo IMGUI=%IMGUI%>> "%LOG%"
echo MINHOOK=%MINHOOK%>> "%LOG%"
rem a missing toolchain part fails here with its name, instead of as a cryptic compiler error
for %%p in ("%TC%\clang++.exe" "%IMGUI%\imgui.cpp" "%MINHOOK%\include\MinHook.h") do if not exist "%%~p" (
  echo Missing: %%~p>> "%LOG%"
  echo Unpack llvm-mingw, ImGui and MinHook into toolchain\ here, or set TC / IMGUI / MINHOOK.>> "%LOG%"
  cmd /c exit /b 2
  goto :err
)

echo [1/3] proxy vulkan-1.dll
echo == [1/3] proxy>> "%LOG%"
rem version resource: lets the installer recognise our vulkan-1.dll and show its version
"%RC%" -I"%HERE%src\proxy" "%HERE%src\proxy\proxy.rc" -O coff -o "%OUT%\proxy_res.o" >> "%LOG%" 2>&1 || goto :err
"%CC%" -O2 -shared -o "%OUT%\vulkan-1.dll" ^
  "%HERE%src\proxy\proxy.c" "%HERE%src\proxy\exports.S" "%HERE%src\proxy\exports_gen.c" "%HERE%src\proxy\vulkan-1.def" "%OUT%\proxy_res.o" ^
  -Wl,--kill-at -nostartfiles -Wl,-e,DllMain -lkernel32 -s >> "%LOG%" 2>&1 || goto :err

echo [2/3] minhook
echo == [2/3] minhook>> "%LOG%"
if not exist "%OUT%\mh" mkdir "%OUT%\mh"
set "MHOBJ="
for %%f in ("%MINHOOK%\src\*.c" "%MINHOOK%\src\hde\*.c") do (
  echo %%~nxf>> "%LOG%"
  "%CC%" -O2 -c -I"%MINHOOK%\include" -o "%OUT%\mh\%%~nf.o" "%%f" >> "%LOG%" 2>&1 || goto :err
  set MHOBJ=!MHOBJ! "%OUT%\mh\%%~nf.o"
)
if not defined MHOBJ (
  echo MinHook sources not found under "%MINHOOK%\src">> "%LOG%"
  goto :err
)

rem -static: llvm-mingw links libc++.dll / libunwind.dll dynamically by default, and the game folder has
rem neither, so the plugin would silently fail to load.  --exclude-all-symbols: export nothing.
echo [3/3] plugin tf3snap.dll  (compiles ImGui too: about 1-2 minutes)
echo == [3/3] plugin>> "%LOG%"
"%RC%" -I"%HERE%src\plugin" "%HERE%src\plugin\tf3snap.rc" -O coff -o "%OUT%\tf3snap_res.o" >> "%LOG%" 2>&1 || goto :err
"%CLANG%" -O2 -shared -std=c++17 -o "%OUT%\tf3snap.dll" ^
  "%HERE%src\plugin\main.cpp" "%HERE%src\plugin\sigscan.cpp" ^
  "%HERE%src\plugin\hooks.cpp" "%HERE%src\plugin\config.cpp" "%HERE%src\plugin\log.cpp" ^
  "%HERE%src\plugin\overlay.cpp" "%HERE%src\plugin\text.cpp" ^
  "%IMGUI%\imgui.cpp" "%IMGUI%\imgui_draw.cpp" "%IMGUI%\imgui_tables.cpp" "%IMGUI%\imgui_widgets.cpp" ^
  "%IMGUI%\backends\imgui_impl_win32.cpp" "%IMGUI%\backends\imgui_impl_dx11.cpp" ^
  !MHOBJ! "%OUT%\tf3snap_res.o" ^
  -I"%HERE%src\plugin" -I"%MINHOOK%\include" -I"%IMGUI%" -I"%IMGUI%\backends" ^
  -static -Wl,--exclude-all-symbols ^
  -ld3d11 -ldxgi -ld3dcompiler -lgdi32 -ldwmapi -lole32 -s >> "%LOG%" 2>&1 || goto :err

echo == OK>> "%LOG%"
echo.
echo OK. Output in %OUT% :  vulkan-1.dll  +  tf3snap.dll
echo Install into the game:   deploy.bat   (or copy vulkan-1.dll next to TransportFever3.exe
echo                          and tf3snap.dll into a "tf3snap" folder there)
echo Make the release zip:    package.bat   (into the built-output folder next to this one)
echo.
pause
exit /b 0

:err
echo.
echo BUILD FAILED (errorlevel %errorlevel%) - compiler output:
echo ------------------------------------------------------------------
type "%LOG%"
echo ------------------------------------------------------------------
echo (saved in %LOG%)
echo.
pause
exit /b 1
