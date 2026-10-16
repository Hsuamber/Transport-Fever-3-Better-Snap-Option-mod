@echo off
setlocal EnableExtensions DisableDelayedExpansion
rem Make the release folder + zip from build\ (run build.bat first), in the built-output folder next to this
rem source folder (dist\ here if there is none).  -Force packs a build whose version does not match
rem src\plugin\version.h (testing only).
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\package.ps1" %*
set "RC=%errorlevel%"
echo.
pause
exit /b %RC%
