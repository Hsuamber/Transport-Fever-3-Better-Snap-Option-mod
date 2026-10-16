@echo off
setlocal EnableExtensions DisableDelayedExpansion
rem Better Snap Option - install / update / uninstall / error report.  The work is done by tools\install.ps1.
rem Without arguments it asks; arguments are passed through (see the top of install.ps1).
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\install.ps1" %*
set "TF3_EXIT=%errorlevel%"
if not "%~1"=="" exit /b %TF3_EXIT%
echo.
pause
exit /b %TF3_EXIT%
