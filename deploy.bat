@echo off
setlocal EnableExtensions DisableDelayedExpansion
rem Developer shortcut: install what build.bat just made (build\*.dll) with the release installer.
rem The game folder is remembered in deploy.local.txt here (asked the first time).  Arguments pass through,
rem e.g.  deploy.bat -GameDir "D:\Games\Transport Fever 3"     or     deploy.bat -Action Report
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\deploy.ps1" %*
set "RC=%errorlevel%"
echo.
pause
exit /b %RC%
