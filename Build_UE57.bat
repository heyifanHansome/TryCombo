@echo off
chcp 65001 >nul

set UE_ROOT=D:\UE_5.7
set PROJECT_ROOT=G:\TryComboWepon
set PROJECT_NAME=TryComboWepon
set PROJECT_FILE=%PROJECT_ROOT%\%PROJECT_NAME%.uproject

rem 优先使用 UE 自带 DotNet，不用系统 C:\Program Files\dotnet
set DOTNET_EXE=%UE_ROOT%\Engine\Binaries\ThirdParty\DotNet\8.0.412\win-x64\dotnet.exe
set DOTNET_ROOT=%UE_ROOT%\Engine\Binaries\ThirdParty\DotNet\8.0.412\win-x64

set PATH=%DOTNET_ROOT%;%PATH%

echo UE_ROOT=%UE_ROOT%
echo PROJECT_FILE=%PROJECT_FILE%
echo DOTNET_EXE=%DOTNET_EXE%

"%DOTNET_EXE%" --info

"%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" %PROJECT_NAME%Editor Win64 Development -Project="%PROJECT_FILE%" -WaitMutex

pause