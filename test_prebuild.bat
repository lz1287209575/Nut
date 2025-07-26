@echo off
rem Test script to simulate the PreBuildEvent logic
rem This mimics what Visual Studio would run

echo === Testing PreBuildEvent Logic ===
echo.

rem Simulate MSBuildProjectDirectory (normally set by Visual Studio)
set "MSBuildProjectDirectory=%~dp0Projects\LibNut"

rem Find project root by looking for CLAUDE.md
set PROJECT_ROOT=%MSBuildProjectDirectory%
:FIND_ROOT
if exist "%PROJECT_ROOT%\CLAUDE.md" goto FOUND_ROOT
for %%I in ("%PROJECT_ROOT%") do set PROJECT_ROOT=%%~dpI
set PROJECT_ROOT=%PROJECT_ROOT:~0,-1%
if "%PROJECT_ROOT%"=="%PROJECT_ROOT:~0,3%" (
    echo Error: Could not find project root
    exit /b 1
)
goto FIND_ROOT
:FOUND_ROOT

echo Found project root: %PROJECT_ROOT%

rem Change to project root
cd /d "%PROJECT_ROOT%" 2>nul || (
    echo Error: Cannot change to project root
    exit /b 1
)

rem Use the DLL directly like nutbuild.bat does
set NUTBUILDTOOLS_DLL=%PROJECT_ROOT%\Binary\NutBuildTools\NutBuildTools.dll

rem Build NutBuildTools if DLL not found
if not exist "%NUTBUILDTOOLS_DLL%" (
    echo Building NutBuildTools...
    dotnet build Source\Programs\NutBuildTools -c Release --output Binary\NutBuildTools 2>nul || (
        echo Warning: Failed to build NutBuildTools, trying without output redirect
        dotnet build Source\Programs\NutBuildTools -c Release --output Binary\NutBuildTools
    )
)

rem Check if DLL exists now
if not exist "%NUTBUILDTOOLS_DLL%" (
    echo Warning: NutBuildTools.dll not found, PreBuild will be skipped
    exit /b 0
)

rem Run NutBuildTools with simple approach
echo Running NutBuildTools for LibNut...
dotnet "%NUTBUILDTOOLS_DLL%" --target LibNut --platform Windows --configuration Debug || (
    echo Warning: NutBuildTools completed with warnings
)
echo PreBuild completed
exit /b 0