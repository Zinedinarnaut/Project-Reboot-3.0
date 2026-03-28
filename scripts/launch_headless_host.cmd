@echo off
setlocal

if "%~1"=="" (
  echo Usage: launch_headless_host.cmd "C:\Path\To\FortniteClient-Win64-Shipping.exe" [args...]
  exit /b 1
)

if "%PR_PROFILE%"=="" set PR_PROFILE=stw
if "%PR_MODE%"=="" set PR_MODE=headless
if "%PR_GUI%"=="" set PR_GUI=0
if "%PR_NO_MCP%"=="" set PR_NO_MCP=1
if "%PR_DISABLE_VERBOSE_FORT_LOGS%"=="" set PR_DISABLE_VERBOSE_FORT_LOGS=1

echo [Project Reboot] profile=%PR_PROFILE% mode=%PR_MODE% gui=%PR_GUI% no_mcp=%PR_NO_MCP%

"%~1" %2 %3 %4 %5 %6 %7 %8 %9
