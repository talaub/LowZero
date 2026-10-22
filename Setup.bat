@echo off
setlocal
cd /d "%~dp0"
set LOG=build\setup.log
if not exist build mkdir build
echo Setup LowEngine installation
echo Please enter an ID for this LowEngine installation.
set /p "id=Enter ID: "

echo Configuring...
cmake -S . -B build -Wno-dev > "%LOG%" 2>&1 || goto :failed

echo Building low cli...
cmake --build build --target LowCli LowShim --config Release > "%LOG%" 2>&1 || goto :failed

echo Installing...
cmake --install build --config Release --component tools --prefix . > "%LOG%" 2>&1 || goto :failed
cmake --install build --config Release --component shim --prefix "%LOCALAPPDATA%\LowEngine\bin" > "%LOG%" 2>&1 || goto :failed

rem Only touches the user PATH (setx would truncate it and merge in the system PATH)
powershell -NoProfile -Command "$d = Join-Path $env:LOCALAPPDATA 'LowEngine\bin'; $p = [Environment]::GetEnvironmentVariable('Path', 'User'); if (-not $p) { $p = '' }; if (($p -split ';') -notcontains $d) { [Environment]::SetEnvironmentVariable('Path', ($p.TrimEnd(';') + ';' + $d).TrimStart(';'), 'User'); Write-Host 'Added' $d 'to your PATH, open a new terminal to use low' }" || goto :failed

bin\low.exe engine register %id% -v %*
exit /b %errorlevel%

:failed
type "%LOG%"
echo.
echo Setup failed, full log: %LOG%
exit /b 1
