"""
Visual Studio 项目生成器

生成 Visual Studio 项目文件 (.vcxproj)
"""

import logging
from pathlib import Path
from typing import List

from Tools.ProjectGenerator.generators.base_generator import BaseGenerator
from Tools.ProjectGenerator.core.project_info import ProjectInfo, FileGroup, ProjectType


logger = logging.getLogger(__name__)


class VcxprojGenerator(BaseGenerator):
    """Visual Studio 项目生成器"""
    
    @property
    def FileExtension(self) -> str:
        return ".vcxproj"
    
    @property
    def GeneratorName(self) -> str:
        return "Visual Studio"
    
    def GenerateProject(self, project_info: ProjectInfo) -> Path:
        """生成 Visual Studio 项目文件"""
        if project_info.is_csharp:
            logger.info(f"跳过 C# 项目（已有 .csproj）: {project_info.name}")
            return None
        
        # 创建项目文件路径
        projects_dir = self.project_root / "Projects" / project_info.group_name
        project_file = projects_dir / f"{project_info.name}.vcxproj"
        filters_file = projects_dir / f"{project_info.name}.vcxproj.filters"
        self._EnsureOutputDirectory(project_file)
        
        # 生成项目内容
        content = self._GenerateVcxprojContent(project_info, project_file)
        filters_content = self._GenerateFiltersContent(project_info, project_file)
        
        # 写入文件
        project_file.write_text(content, encoding='utf-8')
        filters_file.write_text(filters_content, encoding='utf-8')
        
        logger.info(f"生成 Visual Studio 项目: {project_file}")
        logger.info(f"生成 Visual Studio 过滤器: {filters_file}")
        return project_file
    
    def _GenerateVcxprojContent(self, project_info: ProjectInfo, project_file: Path) -> str:
        """生成 vcxproj 文件内容"""
        lines = []
        
        # XML 头和项目开始
        lines.extend([
            '<?xml version="1.0" encoding="utf-8"?>',
            '<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">'
        ])
        
        # 项目配置
        self._AddProjectConfigurations(lines)
        
        # 全局属性
        self._AddGlobalProperties(lines, project_info)
        
        # 导入默认属性
        lines.append('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />')
        
        # 配置属性
        self._AddConfigurationProperties(lines, project_info)
        
        # 导入 C++ 属性
        lines.append('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />')
        
        # 扩展设置
        lines.extend([
            '  <ImportGroup Label="ExtensionSettings">',
            '  </ImportGroup>',
            '  <ImportGroup Label="Shared">',
            '  </ImportGroup>'
        ])
        
        # 属性表
        self._AddPropertySheets(lines)
        
        # 用户宏
        lines.append('  <PropertyGroup Label="UserMacros" />')
        
        # 属性
        self._AddProperties(lines)
        
        # 项目定义组
        self._AddItemDefinitionGroups(lines, project_info)
        
        # 文件项目组
        self._AddFileItemGroups(lines, project_info, project_file)
        
        # 导入 C++ targets
        lines.append('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" />')
        
        # 扩展 targets
        lines.extend([
            '  <ImportGroup Label="ExtensionTargets">',
            '  </ImportGroup>'
        ])
        
        # 项目结束
        lines.append('</Project>')
        
        return '\n'.join(lines)
    
    def _AddProjectConfigurations(self, lines: List[str]):
        """添加项目配置"""
        lines.extend([
            '  <ItemGroup Label="ProjectConfigurations">',
            '    <ProjectConfiguration Include="Debug|x64">',
            '      <Configuration>Debug</Configuration>',
            '      <Platform>x64</Platform>',
            '    </ProjectConfiguration>',
            '    <ProjectConfiguration Include="Release|x64">',
            '      <Configuration>Release</Configuration>',
            '      <Platform>x64</Platform>',
            '    </ProjectConfiguration>',
            '  </ItemGroup>'
        ])
    
    def _AddGlobalProperties(self, lines: List[str], project_info: ProjectInfo):
        """添加全局属性"""
        project_guid = self.uuid_generator.GenerateGuid(project_info.name)
        
        lines.extend([
            '  <PropertyGroup Label="Globals">',
            '    <VCProjectVersion>16.0</VCProjectVersion>',
            '    <Keyword>Win32Proj</Keyword>',
            f'    <ProjectGuid>{project_guid}</ProjectGuid>',
            f'    <RootNamespace>{project_info.name}</RootNamespace>',
            '    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>',
            '  </PropertyGroup>'
        ])
    
    def _AddConfigurationProperties(self, lines: List[str], project_info: ProjectInfo):
        """添加配置属性"""
        # 确定配置类型
        if project_info.is_executable:
            config_type = "Application"
        else:
            config_type = "StaticLibrary"
        
        # Debug 配置
        lines.extend([
            '  <PropertyGroup Condition="\'$(Configuration)|$(Platform)\'==\'Debug|x64\'" Label="Configuration">',
            f'    <ConfigurationType>{config_type}</ConfigurationType>',
            '    <UseDebugLibraries>true</UseDebugLibraries>',
            '    <PlatformToolset>v143</PlatformToolset>',
            '    <CharacterSet>Unicode</CharacterSet>',
            '  </PropertyGroup>'
        ])
        
        # Release 配置
        lines.extend([
            '  <PropertyGroup Condition="\'$(Configuration)|$(Platform)\'==\'Release|x64\'" Label="Configuration">',
            f'    <ConfigurationType>{config_type}</ConfigurationType>',
            '    <UseDebugLibraries>false</UseDebugLibraries>',
            '    <PlatformToolset>v143</PlatformToolset>',
            '    <WholeProgramOptimization>true</WholeProgramOptimization>',
            '    <CharacterSet>Unicode</CharacterSet>',
            '  </PropertyGroup>'
        ])
    
    def _AddPropertySheets(self, lines: List[str]):
        """添加属性表"""
        for config in ['Debug', 'Release']:
            lines.extend([
                f'  <ImportGroup Label="PropertySheets" Condition="\'$(Configuration)|$(Platform)\'==\'{config}|x64\'">',
                '    <Import Project="$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props" Condition="exists(\'$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\')" Label="LocalAppDataPlatform" />',
                '  </ImportGroup>'
            ])
    
    def _AddProperties(self, lines: List[str]):
        """添加属性"""
        lines.extend([
            '  <PropertyGroup Condition="\'$(Configuration)|$(Platform)\'==\'Debug|x64\'">',
            '    <LinkIncremental>true</LinkIncremental>',
            '  </PropertyGroup>',
            '  <PropertyGroup Condition="\'$(Configuration)|$(Platform)\'==\'Release|x64\'">',
            '    <LinkIncremental>false</LinkIncremental>',
            '  </PropertyGroup>'
        ])
    
    def _AddItemDefinitionGroups(self, lines: List[str], project_info: ProjectInfo):
        """添加项目定义组"""
        # Debug 配置
        lines.extend([
            '  <ItemDefinitionGroup Condition="\'$(Configuration)|$(Platform)\'==\'Debug|x64\'">',
            '    <PreBuildEvent>',
            '      <Command>',
            f'<![CDATA[{self._GenerateNutBuildCommand(project_info, "Debug")}]]>',
            '      </Command>',
            '      <Message>Running NutBuildTools...</Message>',
            '    </PreBuildEvent>',
            '    <ClCompile>',
            '      <WarningLevel>Level3</WarningLevel>',
            '      <SDLCheck>true</SDLCheck>',
            '      <PreprocessorDefinitions>_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>',
            '      <ConformanceMode>true</ConformanceMode>',
            '      <LanguageStandard>stdcpp20</LanguageStandard>',
            '      <AdditionalIncludeDirectories>$(ProjectDir);$(ProjectDir)Sources;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>',
            '    </ClCompile>',
            '    <Link>',
            '      <SubSystem>Console</SubSystem>',
            '      <GenerateDebugInformation>true</GenerateDebugInformation>',
            '    </Link>',
            '  </ItemDefinitionGroup>'
        ])
        
        # Release 配置
        lines.extend([
            '  <ItemDefinitionGroup Condition="\'$(Configuration)|$(Platform)\'==\'Release|x64\'">',
            '    <PreBuildEvent>',
            '      <Command>',
            f'<![CDATA[{self._GenerateNutBuildCommand(project_info, "Release")}]]>',
            '      </Command>',
            '      <Message>Running NutBuildTools...</Message>',
            '    </PreBuildEvent>',
            '    <ClCompile>',
            '      <WarningLevel>Level3</WarningLevel>',
            '      <FunctionLevelLinking>true</FunctionLevelLinking>',
            '      <IntrinsicFunctions>true</IntrinsicFunctions>',
            '      <SDLCheck>true</SDLCheck>',
            '      <PreprocessorDefinitions>NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>',
            '      <ConformanceMode>true</ConformanceMode>',
            '      <LanguageStandard>stdcpp20</LanguageStandard>',
            '      <AdditionalIncludeDirectories>$(ProjectDir);$(ProjectDir)Sources;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>',
            '    </ClCompile>',
            '    <Link>',
            '      <SubSystem>Console</SubSystem>',
            '      <EnableCOMDATFolding>true</EnableCOMDATFolding>',
            '      <OptimizeReferences>true</OptimizeReferences>',
            '      <GenerateDebugInformation>true</GenerateDebugInformation>',
            '    </Link>',
            '  </ItemDefinitionGroup>'
        ])
    
    def _GenerateNutBuildCommand(self, project_info: ProjectInfo, configuration: str) -> str:
        """生成NutBuild命令（Windows批处理版本）"""
        command_lines = [
            '@echo off',
            'echo === NutBuild Debug Info ===',
            'echo Visual Studio MSBuildProjectDirectory: $(MSBuildProjectDirectory)',
            '',
            'rem Find the real project root (should contain CLAUDE.md)',
            'set PROJECT_ROOT=$(MSBuildProjectDirectory)',
            ':FIND_ROOT',
            'if exist "%PROJECT_ROOT%\\CLAUDE.md" goto FOUND_ROOT',
            'for %%I in ("%PROJECT_ROOT%") do set PROJECT_ROOT=%%~dpI',
            'set PROJECT_ROOT=%PROJECT_ROOT:~0,-1%',
            'if "%PROJECT_ROOT%"=="%PROJECT_ROOT:~0,3%" (',
            '    echo Error: Could not find project root (CLAUDE.md not found)',
            '    exit /b 1',
            ')',
            'goto FIND_ROOT',
            ':FOUND_ROOT',
            'echo Found project root: %PROJECT_ROOT%',
            'cd /d "%PROJECT_ROOT%"',
            '',
            'rem NutBuildTools binary path',
            'set NUTBUILD_BINARY=%PROJECT_ROOT%\\Binary\\NutBuildTools\\NutBuildTools.exe',
            'echo Looking for binary at: %NUTBUILD_BINARY%',
            '',
            'rem Check if NutBuildTools binary exists',
            'if exist "%NUTBUILD_BINARY%" (',
            '    echo NutBuildTools binary found',
            '    goto RUN_NUTBUILD',
            ')',
            '',
            'echo NutBuildTools binary not found, building...',
            '',
            'rem Find dotnet executable',
            'set DOTNET_PATH=',
            'where dotnet &gt;nul 2&gt;&amp;1',
            'if %ERRORLEVEL%==0 (',
            '    set DOTNET_PATH=dotnet',
            ') else (',
            '    if exist "C:\\Program Files\\dotnet\\dotnet.exe" (',
            '        set DOTNET_PATH=C:\\Program Files\\dotnet\\dotnet.exe',
            '    ) else if exist "%ProgramFiles(x86)%\\dotnet\\dotnet.exe" (',
            '        set DOTNET_PATH=%ProgramFiles(x86)%\\dotnet\\dotnet.exe',
            '    ) else (',
            '        echo Error: dotnet not found',
            '        exit /b 1',
            '    )',
            ')',
            '',
            'echo Using dotnet at: %DOTNET_PATH%',
            '',
            'rem Build NutBuildTools',
            '"%DOTNET_PATH%" publish Source\\Programs\\NutBuildTools -c Release -o Binary\\NutBuildTools',
            '',
            'if not exist "%NUTBUILD_BINARY%" (',
            '    echo Error: Failed to build NutBuildTools',
            '    exit /b 1',
            ')',
            'echo NutBuildTools built successfully',
            '',
            ':RUN_NUTBUILD',
            'rem Run NutBuildTools',
            f'echo Running NutBuildTools for target {project_info.name}...',
            f'"%NUTBUILD_BINARY%" --target {project_info.name} --platform Windows --configuration {configuration}',
        ]
        return '\n'.join(command_lines)
    
    def _GetRelativePath(self, target_path: Path, base_path: Path) -> str:
        """获取相对路径（Visual Studio 特定版本）"""
        try:
            # 确保都是绝对路径
            target_path = target_path.resolve()
            base_path = base_path.resolve()
            
            # 计算相对路径
            relative_path = target_path.relative_to(base_path)
            return str(relative_path).replace('/', '\\')
        except ValueError:
            # 如果无法计算直接相对路径，尝试通过公共祖先计算
            try:
                # 寻找公共祖先
                common_parts = []
                target_parts = target_path.parts
                base_parts = base_path.parts
                
                for i, (t_part, b_part) in enumerate(zip(target_parts, base_parts)):
                    if t_part == b_part:
                        common_parts.append(t_part)
                    else:
                        break
                
                if common_parts:
                    # 计算需要返回的层数
                    back_count = len(base_parts) - len(common_parts)
                    forward_parts = target_parts[len(common_parts):]
                    
                    # 构建相对路径
                    relative_parts = ['..'] * back_count + list(forward_parts)
                    return '\\'.join(relative_parts)
                else:
                    # 没有公共祖先，返回绝对路径
                    return str(target_path)
            except:
                return str(target_path)

    def _AddFileItemGroups(self, lines: List[str], project_info: ProjectInfo, project_file: Path):
        """添加文件项目组"""
        # 使用项目文件的父目录计算相对路径
        project_dir = project_file.parent
        
        # 添加源文件
        source_files = project_info.GetSourceFiles()
        if source_files:
            lines.append('  <ItemGroup>')
            for file_info in source_files:
                relative_path = self._GetRelativePath(file_info.path, project_file.parent)
                lines.append(f'    <ClCompile Include="{relative_path}" />')
            lines.append('  </ItemGroup>')
        
        # 添加头文件
        header_files = project_info.GetHeaderFiles()
        if header_files:
            lines.append('  <ItemGroup>')
            for file_info in header_files:
                relative_path = self._GetRelativePath(file_info.path, project_file.parent)
                lines.append(f'    <ClInclude Include="{relative_path}" />')
            lines.append('  </ItemGroup>')
        
        # 添加其他文件（Meta、Config 等）
        other_files = []
        for group in [FileGroup.META, FileGroup.CONFIGS]:
            other_files.extend(project_info.files[group])
        
        if other_files:
            lines.append('  <ItemGroup>')
            for file_info in other_files:
                relative_path = self._GetRelativePath(file_info.path, project_file.parent)
                lines.append(f'    <None Include="{relative_path}" />')
            lines.append('  </ItemGroup>')
    
    def _GenerateFiltersContent(self, project_info: ProjectInfo, project_file: Path) -> str:
        """生成 vcxproj.filters 文件内容"""
        lines = []
        
        # XML 头和项目开始
        lines.extend([
            '<?xml version="1.0" encoding="utf-8"?>',
            '<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">'
        ])
        
        # 定义过滤器（文件夹）
        lines.extend([
            '  <ItemGroup>',
            '    <Filter Include="Headers">',
            '      <UniqueIdentifier>{93995380-89BD-4b04-88EB-625FBE52EBFB}</UniqueIdentifier>',
            '      <Extensions>h;hh;hpp;hxx;h++;hm;inl;inc;ipp;xsd</Extensions>',
            '    </Filter>',
            '    <Filter Include="Sources">',
            '      <UniqueIdentifier>{4FC737F1-C7A5-4376-A066-2A32D752A2FF}</UniqueIdentifier>',
            '      <Extensions>cpp;c;cc;cxx;c++;cppm;ixx;def;odl;idl;hpj;bat;asm;asmx</Extensions>',
            '    </Filter>',
            '    <Filter Include="Configs">',
            '      <UniqueIdentifier>{67DA6AB6-F800-4c08-8B7A-83BB121AAD01}</UniqueIdentifier>',
            '      <Extensions>rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx;tiff;tif;png;wav;mfcribbon-ms</Extensions>',
            '    </Filter>',
            '    <Filter Include="Meta">',
            '      <UniqueIdentifier>{50E4BC84-97C0-4d2e-A7E7-F3D35DB497D0}</UniqueIdentifier>',
            '    </Filter>',
            '  </ItemGroup>'
        ])
        
        # 添加源文件到过滤器
        source_files = project_info.GetSourceFiles()
        if source_files:
            lines.append('  <ItemGroup>')
            for file_info in source_files:
                relative_path = self._GetRelativePath(file_info.path, project_file.parent)
                lines.append(f'    <ClCompile Include="{relative_path}">')
                lines.append('      <Filter>Sources</Filter>')
                lines.append('    </ClCompile>')
            lines.append('  </ItemGroup>')
        
        # 添加头文件到过滤器
        header_files = project_info.GetHeaderFiles()
        if header_files:
            lines.append('  <ItemGroup>')
            for file_info in header_files:
                relative_path = self._GetRelativePath(file_info.path, project_file.parent)
                lines.append(f'    <ClInclude Include="{relative_path}">')
                lines.append('      <Filter>Headers</Filter>')
                lines.append('    </ClInclude>')
            lines.append('  </ItemGroup>')
        
        # 添加其他文件到过滤器
        config_files = project_info.files[FileGroup.CONFIGS]
        meta_files = project_info.files[FileGroup.META]
        
        if config_files or meta_files:
            lines.append('  <ItemGroup>')
            
            # 配置文件
            for file_info in config_files:
                relative_path = self._GetRelativePath(file_info.path, project_file.parent)
                lines.append(f'    <None Include="{relative_path}">')
                lines.append('      <Filter>Configs</Filter>')
                lines.append('    </None>')
            
            # Meta 文件
            for file_info in meta_files:
                relative_path = self._GetRelativePath(file_info.path, project_file.parent)
                lines.append(f'    <None Include="{relative_path}">')
                lines.append('      <Filter>Meta</Filter>')
                lines.append('    </None>')
            
            lines.append('  </ItemGroup>')
        
        # 项目结束
        lines.append('</Project>')
        
        return '\n'.join(lines)