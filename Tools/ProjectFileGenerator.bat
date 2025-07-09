@echo off
setlocal enabledelayedexpansion

REM Nut 项目文件生成器 (Windows 版本)
REM 用于在 Setup 时创建基础项目文件，在 GenerateProjectFiles 时更新项目列表

set PROJECT_ROOT=%~dp0..
cd /d "%PROJECT_ROOT%"

REM 生成 MD5 哈希作为 GUID
:generate_guid
set "name=%~1"
for /f "tokens=*" %%i in ('echo %name% ^| certutil -hashfile - ^| findstr /v "CertUtil" ^| findstr /v "MD5" ^| findstr /v "SHA1" ^| findstr /v "SHA256"') do set "hash=%%i"
set "hash=!hash: =!"
set "guid={!hash:~0,8!-!hash:~8,4!-!hash:~12,4!-!hash:~16,4!-!hash:~20,12!}"
goto :eof

REM 获取项目分组
:get_project_group
set "project_path=%~1"
set "source_dir=%PROJECT_ROOT%\Source"

REM 获取相对于 Source 的路径
for %%i in ("%source_dir%") do set "source_abs=%%~fi"
for %%i in ("%project_path%") do set "project_abs=%%~fi"
set "relative_to_source=!project_abs:%source_abs%\=!"

REM 提取第一级目录名
for /f "tokens=1 delims=\" %%i in ("!relative_to_source!") do set "group=%%i"

REM 如果没有分组，使用 "Other"
if "!group!"=="" set "group=Other"
if "!group!"="." set "group=Other"

goto :eof

REM 检查分组是否存在
:group_exists
set "group=%~1"
set "groups=%~2"
echo !groups! | findstr /c:"|!group!|" >nul
if !errorlevel! equ 0 (
    set "exists=true"
) else (
    set "exists=false"
)
goto :eof

REM 添加分组
:add_group
set "group=%~1"
set "groups=%~2"
call :group_exists "!group!" "!groups!"
if "!exists!"=="false" (
    set "groups=!groups!|!group!|"
)
goto :eof

REM 发现项目
:discover_projects
set "projects="
set "source_dir=%PROJECT_ROOT%\Source"

if not exist "%source_dir%" goto :eof

REM 发现 C++ 项目 (通过 Meta/*.Build.py 文件)
for /r "%source_dir%" %%f in (Meta\*.Build.py) do (
    if exist "%%f" (
        REM 获取项目目录 (Meta 的上级目录)
        for %%d in ("%%~dpf..") do set "project_dir=%%~fd"
        for %%n in ("!project_dir!") do set "project_name=%%~nn"
        
        REM 获取相对路径
        for %%i in ("%PROJECT_ROOT%") do set "root_abs=%%~fi"
        set "relative_path=!project_dir:%root_abs%\=!"
        
        REM 获取分组
        call :get_project_group "!project_dir!"
        
        REM 检查是否已存在
        set "found=false"
        for %%p in (!projects!) do (
            echo %%p | findstr /c:"|!project_name!|" >nul
            if !errorlevel! equ 0 set "found=true"
        )
        
        if "!found!"=="false" (
            if defined projects (
                set "projects=!projects! cpp^|!project_name!^|!relative_path!^|!group!"
            ) else (
                set "projects=cpp^|!project_name!^|!relative_path!^|!group!"
            )
        )
    )
)

REM 发现 C# 项目
for /r "%source_dir%" %%f in (*.csproj) do (
    if exist "%%f" (
        for %%d in ("%%~dpf") do set "project_dir=%%~fd"
        for %%n in ("%%~nf") do set "project_name=%%~nn"
        
        REM 获取相对路径
        for %%i in ("%PROJECT_ROOT%") do set "root_abs=%%~fi"
        set "relative_path=!project_dir:%root_abs%\=!"
        
        REM 获取分组
        call :get_project_group "!project_dir!"
        
        if defined projects (
            set "projects=!projects! csharp^|!project_name!^|!relative_path!^|!group!"
        ) else (
            set "projects=csharp^|!project_name!^|!relative_path!^|!group!"
        )
    )
)

goto :eof

REM 生成 Visual Studio 解决方案
:generate_sln
set "sln_path=%PROJECT_ROOT%\Nut.sln"

REM 创建解决方案文件
(
echo Microsoft Visual Studio Solution File, Format Version 12.00
echo # Visual Studio Version 17
echo VisualStudioVersion = 17.0.31903.59
echo MinimumVisualStudioVersion = 10.0.40219.1
) > "!sln_path!"

REM 按分组组织项目
set "project_configs="
set "nested_projects="

REM 添加项目
for %%p in (%*) do (
    for /f "tokens=1,2,3,4 delims=|" %%a in ("%%p") do (
        set "type=%%a"
        set "name=%%b"
        set "path=%%c"
        set "group=%%d"
        
        call :generate_guid "!name!"
        
        if "!type!"=="csharp" (
            set "project_type_guid={FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}"
        ) else (
            set "project_type_guid={8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}"
        )
        
        echo Project("!project_type_guid!") = "!name!", "!path!", "!guid!" >> "!sln_path!"
        echo EndProject >> "!sln_path!"
        
        set "project_configs=!project_configs! !guid!.Debug^|Any CPU.ActiveCfg = Debug^|Any CPU"
        set "project_configs=!project_configs! !guid!.Debug^|Any CPU.Build.0 = Debug^|Any CPU"
        set "project_configs=!project_configs! !guid!.Release^|Any CPU.ActiveCfg = Release^|Any CPU"
        set "project_configs=!project_configs! !guid!.Release^|Any CPU.Build.0 = Release^|Any CPU"
        
        REM 为嵌套项目准备
        call :generate_guid "Folder_!group!"
        set "group_guid=!guid!"
        set "nested_projects=!nested_projects! !guid! = !group_guid!"
    )
)

REM 添加分组文件夹
for %%p in (%*) do (
    for /f "tokens=4 delims=|" %%d in ("%%p") do (
        set "group=%%d"
        call :generate_guid "Folder_!group!"
        echo Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "!group!", "!group!", "!guid!" >> "!sln_path!"
        echo EndProject >> "!sln_path!"
    )
)

REM 添加全局配置
(
echo Global
echo 	GlobalSection(SolutionConfigurationPlatforms^) = preSolution
echo 		Debug^|Any CPU = Debug^|Any CPU
echo 		Release^|Any CPU = Release^|Any CPU
echo 	EndGlobalSection
echo 	GlobalSection(ProjectConfigurationPlatforms^) = postSolution
) >> "!sln_path!"

REM 添加项目配置
for %%c in (!project_configs!) do (
    echo 		%%c >> "!sln_path!"
)

REM 添加嵌套项目配置
(
echo 	EndGlobalSection
echo 	GlobalSection(NestedProjects^) = preSolution
) >> "!sln_path!"

REM 添加嵌套项目
for %%n in (!nested_projects!) do (
    echo 		%%n >> "!sln_path!"
)

(
echo 	EndGlobalSection
echo 	GlobalSection(SolutionProperties^) = preSolution
echo 		HideSolutionNode = FALSE
echo 	EndGlobalSection
echo EndGlobal
) >> "!sln_path!"

echo ✅ 已生成 Visual Studio 解决方案: !sln_path!
goto :eof

REM 生成 Xcode 项目
:generate_xcodeproj
set "xcode_dir=%PROJECT_ROOT%\Nut.xcodeproj"

if not exist "!xcode_dir!" mkdir "!xcode_dir!"

REM 创建简化的 project.pbxproj
(
echo // !$*UTF8*$!
echo {
echo 	archiveVersion = 1;
echo 	classes = {
echo 	};
echo 	objectVersion = 56;
echo 	objects = {
echo 		/* Begin PBXBuildFile section */
echo 		/* End PBXBuildFile section */
echo 		
echo 		/* Begin PBXFileReference section */
echo 		/* End PBXFileReference section */
echo 		
echo 		/* Begin PBXFrameworksBuildPhase section */
echo 		/* End PBXFrameworksBuildPhase section */
echo 		
echo 		/* Begin PBXGroup section */
) > "!xcode_dir!\project.pbxproj"

REM 添加分组
for %%p in (%*) do (
    for /f "tokens=4 delims=|" %%d in ("%%p") do (
        set "group=%%d"
        echo 		/* !group! Group */ >> "!xcode_dir!\project.pbxproj"
    )
)

(
echo 		/* End PBXGroup section */
echo 		
echo 		/* Begin PBXNativeTarget section */
echo 		/* End PBXNativeTarget section */
echo 		
echo 		/* Begin XCBuildConfiguration section */
echo 		/* End XCBuildConfiguration section */
echo 		
echo 		/* Begin XCConfigurationList section */
echo 		/* End XCConfigurationList section */
echo 	};
echo 	rootObject = 1234567890ABCDEF /* Project object */;
echo }
) >> "!xcode_dir!\project.pbxproj"

echo ✅ 已生成 Xcode 项目: !xcode_dir!
goto :eof

REM 主函数
set "action=%~1"
if "%action%"=="" set "action=generate"

if "%action%"=="setup" (
    echo 🔧 正在创建基础项目文件...
    call :generate_sln
    call :generate_xcodeproj
    echo ✅ 基础项目文件创建完成！
    goto :end
)

if "%action%"=="generate" (
    echo 🔍 正在发现项目...
    call :discover_projects
    set "projects=!projects!"
    
    if "!projects!"=="" (
        echo ⚠️  未发现任何项目
        goto :end
    )
    
    echo 📁 发现项目:
    REM 按分组显示项目
    for %%p in (!projects!) do (
        for /f "tokens=1,2,4 delims=|" %%a in ("%%p") do (
            set "type=%%a"
            set "name=%%b"
            set "group=%%d"
            echo   📂 !group!: !name!(!type!^)
        )
    )
    
    echo.
    echo 🔨 正在生成项目文件...
    call :generate_sln !projects!
    call :generate_xcodeproj !projects!
    echo.
    echo ✅ 项目文件生成完成！
    goto :end
)

echo 用法: %0 [setup^|generate]
echo   setup   - 创建基础项目文件
echo   generate - 发现项目并更新项目文件
exit /b 1

:end 