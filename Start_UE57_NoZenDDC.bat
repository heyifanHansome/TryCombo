@echo off
chcp 65001 >nul

set UE_ROOT=D:\UE_5.7
set PROJECT_FILE=G:\TryComboWepon\TryComboWepon.uproject

rem UE 5.7 installed builds may fail if Zen DDC does not become healthy.
rem This graph disables local Zen and uses the normal writable file DDC instead.
start "" "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT_FILE%" -ddc=InstalledNoZenLocalFallback
