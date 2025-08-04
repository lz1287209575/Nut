using System.Text;
using NutProjectFileGenerator.Models;

namespace NutProjectFileGenerator.Generators
{
    /// <summary>
    /// Visual Studio 解决方案和项目文件生成器
    /// </summary>
    public class VisualStudioGenerator : ProjectFileGeneratorBase
    {
        public override string Name => "VisualStudio";

        public override string Description => "Generate Visual Studio solution and project files (.sln, .vcxproj)";

        public override List<string> SupportedPlatforms => new() { "Windows" };

        private readonly Dictionary<string, string> projectGuids = new();
        private readonly string solutionGuid = Guid.NewGuid().ToString("B").ToUpper();

        public override bool CanGenerate()
        {
            return OperatingSystem.IsWindows() || Environment.GetEnvironmentVariable("FORCE_VS_GEN") == "1";
        }

        public override async Task<bool> GenerateAsync(SolutionInfo solutionInfo, string outputDirectory)
        {
            try
            {
                // 为每个项目生成GUID
                foreach (var project in solutionInfo.Projects)
                {
                    projectGuids[project.Name] = Guid.NewGuid().ToString("B").ToUpper();
                }

                // 生成解决方案文件
                await GenerateSolutionFileAsync(solutionInfo, outputDirectory);

                // 为每个项目生成项目文件
                foreach (var project in solutionInfo.Projects)
                {
                    await GenerateProjectFileAsync(project, solutionInfo, outputDirectory);
                }

                // 生成属性文件
                await GeneratePropertySheetsAsync(solutionInfo, outputDirectory);

                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"生成Visual Studio项目失败: {ex.Message}");
                return false;
            }
        }

        public override List<string> GetGeneratedFiles(SolutionInfo solutionInfo, string outputDirectory)
        {
            var files = new List<string>
            {
                Path.Combine(outputDirectory, $"{solutionInfo.Name}.sln"),
                Path.Combine(outputDirectory, "Common.props"),
                Path.Combine(outputDirectory, "Debug.props"),
                Path.Combine(outputDirectory, "Release.props")
            };

            foreach (var project in solutionInfo.Projects)
            {
                files.Add(Path.Combine(outputDirectory, $"{project.Name}.vcxproj"));
                files.Add(Path.Combine(outputDirectory, $"{project.Name}.vcxproj.filters"));
            }

            return files;
        }

        /// <summary>
        /// 生成解决方案文件 (.sln)
        /// </summary>
        private async Task GenerateSolutionFileAsync(SolutionInfo solutionInfo, string outputDirectory)
        {
            var sb = new StringBuilder();

            // 解决方案头部
            sb.AppendLine("Microsoft Visual Studio Solution File, Format Version 12.00");
            sb.AppendLine("# Visual Studio Version 17");
            sb.AppendLine("VisualStudioVersion = 17.0.31903.59");
            sb.AppendLine("MinimumVisualStudioVersion = 10.0.40219.1");

            // 项目声明
            foreach (var project in solutionInfo.Projects)
            {
                var projectTypeGuid = GetProjectTypeGuid(project.Type);
                var projectGuid = projectGuids[project.Name];
                var projectPath = $"{project.Name}.vcxproj";

                sb.AppendLine($"Project(\"{projectTypeGuid}\") = \"{project.Name}\", \"{projectPath}\", \"{projectGuid}\"");
                sb.AppendLine("EndProject");
            }

            // 解决方案文件夹（可选）
            var solutionFolderGuid = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}";
            sb.AppendLine($"Project(\"{solutionFolderGuid}\") = \"Solution Items\", \"Solution Items\", \"{Guid.NewGuid().ToString("B").ToUpper()}\"");
            sb.AppendLine("\tProjectSection(SolutionItems) = preProject");
            sb.AppendLine("\t\tCommon.props = Common.props");
            sb.AppendLine("\t\tDebug.props = Debug.props");
            sb.AppendLine("\t\tRelease.props = Release.props");
            sb.AppendLine("\tEndProjectSection");
            sb.AppendLine("EndProject");

            // 全局配置
            sb.AppendLine("Global");
            
            // 解决方案配置平台
            sb.AppendLine("\tGlobalSection(SolutionConfigurationPlatforms) = preSolution");
            foreach (var config in solutionInfo.Projects.SelectMany(p => p.SupportedConfigurations).Distinct())
            {
                sb.AppendLine($"\t\t{config}|x64 = {config}|x64");
                sb.AppendLine($"\t\t{config}|Win32 = {config}|Win32");
            }
            sb.AppendLine("\tEndGlobalSection");

            // 项目配置平台
            sb.AppendLine("\tGlobalSection(ProjectConfigurationPlatforms) = postSolution");
            foreach (var project in solutionInfo.Projects)
            {
                var projectGuid = projectGuids[project.Name];
                foreach (var config in project.SupportedConfigurations)
                {
                    sb.AppendLine($"\t\t{projectGuid}.{config}|x64.ActiveCfg = {config}|x64");
                    sb.AppendLine($"\t\t{projectGuid}.{config}|x64.Build.0 = {config}|x64");
                    sb.AppendLine($"\t\t{projectGuid}.{config}|Win32.ActiveCfg = {config}|Win32");
                    sb.AppendLine($"\t\t{projectGuid}.{config}|Win32.Build.0 = {config}|Win32");
                }
            }
            sb.AppendLine("\tEndGlobalSection");

            // 解决方案属性
            sb.AppendLine("\tGlobalSection(SolutionProperties) = preSolution");
            sb.AppendLine("\t\tHideSolutionNode = FALSE");
            sb.AppendLine("\tEndGlobalSection");

            // 扩展性全局变量
            sb.AppendLine("\tGlobalSection(ExtensibilityGlobals) = postSolution");
            sb.AppendLine($"\t\tSolutionGuid = {solutionGuid}");
            sb.AppendLine("\tEndGlobalSection");

            sb.AppendLine("EndGlobal");

            var solutionPath = Path.Combine(outputDirectory, $"{solutionInfo.Name}.sln");
            await File.WriteAllTextAsync(solutionPath, sb.ToString());
        }

        /// <summary>
        /// 生成项目文件 (.vcxproj)
        /// </summary>
        private async Task GenerateProjectFileAsync(ProjectInfo project, SolutionInfo solutionInfo, string outputDirectory)
        {
            var sb = new StringBuilder();

            // XML头部和项目开始
            sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            sb.AppendLine("<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">");

            // 项目配置
            sb.AppendLine("  <ItemGroup Label=\"ProjectConfigurations\">");
            foreach (var config in project.SupportedConfigurations)
            {
                sb.AppendLine($"    <ProjectConfiguration Include=\"{config}|Win32\">");
                sb.AppendLine($"      <Configuration>{config}</Configuration>");
                sb.AppendLine("      <Platform>Win32</Platform>");
                sb.AppendLine("    </ProjectConfiguration>");
                sb.AppendLine($"    <ProjectConfiguration Include=\"{config}|x64\">");
                sb.AppendLine($"      <Configuration>{config}</Configuration>");
                sb.AppendLine("      <Platform>x64</Platform>");
                sb.AppendLine("    </ProjectConfiguration>");
            }
            sb.AppendLine("  </ItemGroup>");

            // 项目全局属性
            sb.AppendLine("  <PropertyGroup Label=\"Globals\">");
            sb.AppendLine("    <VCProjectVersion>17.0</VCProjectVersion>");
            sb.AppendLine("    <Keyword>Win32Proj</Keyword>");
            sb.AppendLine($"    <ProjectGuid>{projectGuids[project.Name]}</ProjectGuid>");
            sb.AppendLine($"    <RootNamespace>{project.Name}</RootNamespace>");
            sb.AppendLine("    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>");
            sb.AppendLine("  </PropertyGroup>");

            // 导入默认属性
            sb.AppendLine("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />");

            // 配置属性
            foreach (var config in project.SupportedConfigurations)
            {
                foreach (var platform in new[] { "Win32", "x64" })
                {
                    sb.AppendLine($"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='{config}|{platform}'\" Label=\"Configuration\">");
                    
                    var configurationType = project.Type switch
                    {
                        ProjectType.Executable => "Application",
                        ProjectType.StaticLibrary => "StaticLibrary",
                        ProjectType.DynamicLibrary => "DynamicLibrary",
                        _ => "Application"
                    };
                    
                    sb.AppendLine($"    <ConfigurationType>{configurationType}</ConfigurationType>");
                    sb.AppendLine("    <UseDebugLibraries>" + (config == "Debug" ? "true" : "false") + "</UseDebugLibraries>");
                    sb.AppendLine("    <PlatformToolset>v143</PlatformToolset>");
                    sb.AppendLine("    <WholeProgramOptimization>" + (config == "Release" ? "true" : "false") + "</WholeProgramOptimization>");
                    sb.AppendLine("    <CharacterSet>Unicode</CharacterSet>");
                    sb.AppendLine("  </PropertyGroup>");
                }
            }

            // 导入属性文件
            sb.AppendLine("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />");
            sb.AppendLine("  <ImportGroup Label=\"ExtensionSettings\">");
            sb.AppendLine("  </ImportGroup>");

            // 导入用户属性
            sb.AppendLine("  <ImportGroup Label=\"Shared\">");
            sb.AppendLine("  </ImportGroup>");

            foreach (var config in project.SupportedConfigurations)
            {
                foreach (var platform in new[] { "Win32", "x64" })
                {
                    sb.AppendLine($"  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='{config}|{platform}'\">");
                    sb.AppendLine("    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />");
                    sb.AppendLine("    <Import Project=\"Common.props\" />");
                    sb.AppendLine($"    <Import Project=\"{config}.props\" />");
                    sb.AppendLine("  </ImportGroup>");
                }
            }

            sb.AppendLine("  <PropertyGroup Label=\"UserMacros\" />");

            // 配置特定属性
            foreach (var config in project.SupportedConfigurations)
            {
                foreach (var platform in new[] { "Win32", "x64" })
                {
                    sb.AppendLine($"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='{config}|{platform}'\">");
                    
                    var outputDir = Path.Combine(project.OutputDirectory, platform, config).Replace('/', '\\');
                    var intermediateDir = Path.Combine(solutionInfo.IntermediateDirectory, project.Name, platform, config).Replace('/', '\\');
                    
                    sb.AppendLine($"    <OutDir>{outputDir}\\</OutDir>");
                    sb.AppendLine($"    <IntDir>{intermediateDir}\\</IntDir>");
                    sb.AppendLine($"    <TargetName>{project.OutputName}</TargetName>");
                    sb.AppendLine("  </PropertyGroup>");
                }
            }

            // 编译和链接选项
            foreach (var config in project.SupportedConfigurations)
            {
                foreach (var platform in new[] { "Win32", "x64" })
                {
                    sb.AppendLine($"  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='{config}|{platform}'\">");
                    sb.AppendLine("    <ClCompile>");
                    
                    // 包含目录
                    var includeDirs = string.Join(";", project.IncludeDirectories.Concat(solutionInfo.GlobalIncludeDirectories));
                    sb.AppendLine($"      <AdditionalIncludeDirectories>{includeDirs};%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>");
                    
                    // 预处理器定义
                    var defines = string.Join(";", project.PreprocessorDefinitions.Concat(solutionInfo.GlobalPreprocessorDefinitions));
                    if (config == "Debug")
                    {
                        defines += ";_DEBUG";
                    }
                    else
                    {
                        defines += ";NDEBUG";
                    }
                    sb.AppendLine($"      <PreprocessorDefinitions>{defines};%(PreprocessorDefinitions)</PreprocessorDefinitions>");
                    
                    sb.AppendLine("      <WarningLevel>Level3</WarningLevel>");
                    sb.AppendLine("      <FunctionLevelLinking>true</FunctionLevelLinking>");
                    sb.AppendLine("      <IntrinsicFunctions>true</IntrinsicFunctions>");
                    sb.AppendLine("      <SDLCheck>true</SDLCheck>");
                    sb.AppendLine("      <PrecompiledHeader>NotUsing</PrecompiledHeader>");
                    sb.AppendLine("      <ConformanceMode>true</ConformanceMode>");
                    sb.AppendLine("      <LanguageStandard>stdcpp20</LanguageStandard>");
                    
                    if (config == "Debug")
                    {
                        sb.AppendLine("      <Optimization>Disabled</Optimization>");
                        sb.AppendLine("      <RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>");
                    }
                    else
                    {
                        sb.AppendLine("      <Optimization>MaxSpeed</Optimization>");
                        sb.AppendLine("      <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>");
                    }
                    
                    sb.AppendLine("    </ClCompile>");

                    // 链接器选项（仅对可执行文件和动态库）
                    if (project.Type != ProjectType.StaticLibrary)
                    {
                        sb.AppendLine("    <Link>");
                        sb.AppendLine("      <SubSystem>" + (project.Type == ProjectType.Executable ? "Console" : "Windows") + "</SubSystem>");
                        sb.AppendLine("      <EnableCOMDATFolding>true</EnableCOMDATFolding>");
                        sb.AppendLine("      <OptimizeReferences>true</OptimizeReferences>");
                        sb.AppendLine("      <GenerateDebugInformation>" + (config == "Debug" ? "true" : "false") + "</GenerateDebugInformation>");
                        sb.AppendLine("    </Link>");
                    }

                    sb.AppendLine("  </ItemDefinitionGroup>");
                }
            }

            // 源文件
            if (project.SourceFiles.Count > 0)
            {
                sb.AppendLine("  <ItemGroup>");
                foreach (var sourceFile in project.SourceFiles)
                {
                    var relativePath = GetRelativePath(outputDirectory, sourceFile);
                    sb.AppendLine($"    <ClCompile Include=\"{NormalizePath(relativePath, '\\')}\" />");
                }
                sb.AppendLine("  </ItemGroup>");
            }

            // 头文件
            if (project.HeaderFiles.Count > 0)
            {
                sb.AppendLine("  <ItemGroup>");
                foreach (var headerFile in project.HeaderFiles)
                {
                    var relativePath = GetRelativePath(outputDirectory, headerFile);
                    sb.AppendLine($"    <ClInclude Include=\"{NormalizePath(relativePath, '\\')}\" />");
                }
                sb.AppendLine("  </ItemGroup>");
            }

            // 项目依赖
            if (project.Dependencies.Count > 0)
            {
                sb.AppendLine("  <ItemGroup>");
                foreach (var dependency in project.Dependencies)
                {
                    if (solutionInfo.Projects.Any(p => p.Name == dependency))
                    {
                        sb.AppendLine($"    <ProjectReference Include=\"{dependency}.vcxproj\">");
                        sb.AppendLine($"      <Project>{projectGuids.GetValueOrDefault(dependency, Guid.NewGuid().ToString("B").ToUpper())}</Project>");
                        sb.AppendLine("    </ProjectReference>");
                    }
                }
                sb.AppendLine("  </ItemGroup>");
            }

            // 导入目标
            sb.AppendLine("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />");
            sb.AppendLine("  <ImportGroup Label=\"ExtensionTargets\">");
            sb.AppendLine("  </ImportGroup>");

            sb.AppendLine("</Project>");

            var projectPath = Path.Combine(outputDirectory, $"{project.Name}.vcxproj");
            await File.WriteAllTextAsync(projectPath, sb.ToString());

            // 生成过滤器文件
            await GenerateFiltersFileAsync(project, outputDirectory);
        }

        /// <summary>
        /// 生成过滤器文件 (.vcxproj.filters)
        /// </summary>
        private async Task GenerateFiltersFileAsync(ProjectInfo project, string outputDirectory)
        {
            var sb = new StringBuilder();

            sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            sb.AppendLine("<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">");

            // 创建过滤器
            var filters = new HashSet<string>();
            
            // 为源文件创建过滤器
            foreach (var sourceFile in project.SourceFiles)
            {
                var dir = Path.GetDirectoryName(GetRelativePath(outputDirectory, sourceFile))?.Replace('/', '\\');
                if (!string.IsNullOrEmpty(dir))
                {
                    var parts = dir.Split('\\', StringSplitOptions.RemoveEmptyEntries);
                    for (int i = 0; i < parts.Length; i++)
                    {
                        var filter = string.Join("\\", parts.Take(i + 1));
                        filters.Add(filter);
                    }
                }
            }

            // 为头文件创建过滤器
            foreach (var headerFile in project.HeaderFiles)
            {
                var dir = Path.GetDirectoryName(GetRelativePath(outputDirectory, headerFile))?.Replace('/', '\\');
                if (!string.IsNullOrEmpty(dir))
                {
                    var parts = dir.Split('\\', StringSplitOptions.RemoveEmptyEntries);
                    for (int i = 0; i < parts.Length; i++)
                    {
                        var filter = string.Join("\\", parts.Take(i + 1));
                        filters.Add(filter);
                    }
                }
            }

            // 输出过滤器定义
            if (filters.Count > 0)
            {
                sb.AppendLine("  <ItemGroup>");
                foreach (var filter in filters.OrderBy(f => f))
                {
                    sb.AppendLine($"    <Filter Include=\"{filter}\">");
                    sb.AppendLine($"      <UniqueIdentifier>{{{Guid.NewGuid()}}}</UniqueIdentifier>");
                    sb.AppendLine("    </Filter>");
                }
                sb.AppendLine("  </ItemGroup>");
            }

            // 源文件过滤器
            if (project.SourceFiles.Count > 0)
            {
                sb.AppendLine("  <ItemGroup>");
                foreach (var sourceFile in project.SourceFiles)
                {
                    var relativePath = GetRelativePath(outputDirectory, sourceFile);
                    var normalizedPath = NormalizePath(relativePath, '\\');
                    var filter = Path.GetDirectoryName(normalizedPath)?.Replace('/', '\\');
                    
                    sb.AppendLine($"    <ClCompile Include=\"{normalizedPath}\">");
                    if (!string.IsNullOrEmpty(filter))
                    {
                        sb.AppendLine($"      <Filter>{filter}</Filter>");
                    }
                    sb.AppendLine("    </ClCompile>");
                }
                sb.AppendLine("  </ItemGroup>");
            }

            // 头文件过滤器
            if (project.HeaderFiles.Count > 0)
            {
                sb.AppendLine("  <ItemGroup>");
                foreach (var headerFile in project.HeaderFiles)
                {
                    var relativePath = GetRelativePath(outputDirectory, headerFile);
                    var normalizedPath = NormalizePath(relativePath, '\\');
                    var filter = Path.GetDirectoryName(normalizedPath)?.Replace('/', '\\');
                    
                    sb.AppendLine($"    <ClInclude Include=\"{normalizedPath}\">");
                    if (!string.IsNullOrEmpty(filter))
                    {
                        sb.AppendLine($"      <Filter>{filter}</Filter>");
                    }
                    sb.AppendLine("    </ClInclude>");
                }
                sb.AppendLine("  </ItemGroup>");
            }

            sb.AppendLine("</Project>");

            var filtersPath = Path.Combine(outputDirectory, $"{project.Name}.vcxproj.filters");
            await File.WriteAllTextAsync(filtersPath, sb.ToString());
        }

        /// <summary>
        /// 生成属性文件
        /// </summary>
        private async Task GeneratePropertySheetsAsync(SolutionInfo solutionInfo, string outputDirectory)
        {
            // Common.props
            await GenerateCommonPropsAsync(solutionInfo, outputDirectory);
            
            // Debug.props
            await GenerateDebugPropsAsync(solutionInfo, outputDirectory);
            
            // Release.props
            await GenerateReleasePropsAsync(solutionInfo, outputDirectory);
        }

        /// <summary>
        /// 生成通用属性文件
        /// </summary>
        private async Task GenerateCommonPropsAsync(SolutionInfo solutionInfo, string outputDirectory)
        {
            var sb = new StringBuilder();

            sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            sb.AppendLine("<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">");
            sb.AppendLine("  <ImportGroup Label=\"PropertySheets\" />");
            sb.AppendLine("  <PropertyGroup Label=\"UserMacros\">");
            
            var projectRoot = NormalizePath(solutionInfo.SolutionRoot, '\\');
            var thirdPartyDir = NormalizePath(solutionInfo.ThirdPartyDirectory, '\\');
            var binaryDir = NormalizePath(solutionInfo.BinaryDirectory, '\\');
            var intermediateDir = NormalizePath(solutionInfo.IntermediateDirectory, '\\');
            
            sb.AppendLine($"    <NutProjectRoot>{projectRoot}</NutProjectRoot>");
            sb.AppendLine($"    <NutThirdParty>{thirdPartyDir}</NutThirdParty>");
            sb.AppendLine($"    <NutBinary>{binaryDir}</NutBinary>");
            sb.AppendLine($"    <NutIntermediate>{intermediateDir}</NutIntermediate>");
            sb.AppendLine("  </PropertyGroup>");
            
            sb.AppendLine("  <PropertyGroup />");
            sb.AppendLine("  <ItemDefinitionGroup />");
            sb.AppendLine("  <ItemGroup />");
            sb.AppendLine("</Project>");

            await File.WriteAllTextAsync(Path.Combine(outputDirectory, "Common.props"), sb.ToString());
        }

        /// <summary>
        /// 生成Debug属性文件
        /// </summary>
        private async Task GenerateDebugPropsAsync(SolutionInfo solutionInfo, string outputDirectory)
        {
            var sb = new StringBuilder();

            sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            sb.AppendLine("<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">");
            sb.AppendLine("  <ImportGroup Label=\"PropertySheets\" />");
            sb.AppendLine("  <PropertyGroup Label=\"UserMacros\" />");
            sb.AppendLine("  <PropertyGroup />");
            sb.AppendLine("  <ItemDefinitionGroup>");
            sb.AppendLine("    <ClCompile>");
            sb.AppendLine("      <Optimization>Disabled</Optimization>");
            sb.AppendLine("      <RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>");
            sb.AppendLine("      <BasicRuntimeChecks>EnableFastChecks</BasicRuntimeChecks>");
            sb.AppendLine("      <DebugInformationFormat>EditAndContinue</DebugInformationFormat>");
            sb.AppendLine("    </ClCompile>");
            sb.AppendLine("    <Link>");
            sb.AppendLine("      <GenerateDebugInformation>true</GenerateDebugInformation>");
            sb.AppendLine("    </Link>");
            sb.AppendLine("  </ItemDefinitionGroup>");
            sb.AppendLine("  <ItemGroup />");
            sb.AppendLine("</Project>");

            await File.WriteAllTextAsync(Path.Combine(outputDirectory, "Debug.props"), sb.ToString());
        }

        /// <summary>
        /// 生成Release属性文件
        /// </summary>
        private async Task GenerateReleasePropsAsync(SolutionInfo solutionInfo, string outputDirectory)
        {
            var sb = new StringBuilder();

            sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            sb.AppendLine("<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">");
            sb.AppendLine("  <ImportGroup Label=\"PropertySheets\" />");
            sb.AppendLine("  <PropertyGroup Label=\"UserMacros\" />");
            sb.AppendLine("  <PropertyGroup />");
            sb.AppendLine("  <ItemDefinitionGroup>");
            sb.AppendLine("    <ClCompile>");
            sb.AppendLine("      <Optimization>MaxSpeed</Optimization>");
            sb.AppendLine("      <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>");
            sb.AppendLine("      <FunctionLevelLinking>true</FunctionLevelLinking>");
            sb.AppendLine("      <IntrinsicFunctions>true</IntrinsicFunctions>");
            sb.AppendLine("      <WholeProgramOptimization>true</WholeProgramOptimization>");
            sb.AppendLine("    </ClCompile>");
            sb.AppendLine("    <Link>");
            sb.AppendLine("      <EnableCOMDATFolding>true</EnableCOMDATFolding>");
            sb.AppendLine("      <OptimizeReferences>true</OptimizeReferences>");
            sb.AppendLine("      <LinkTimeCodeGeneration>UseLinkTimeCodeGeneration</LinkTimeCodeGeneration>");
            sb.AppendLine("    </Link>");
            sb.AppendLine("  </ItemDefinitionGroup>");
            sb.AppendLine("  <ItemGroup />");
            sb.AppendLine("</Project>");

            await File.WriteAllTextAsync(Path.Combine(outputDirectory, "Release.props"), sb.ToString());
        }

        /// <summary>
        /// 获取项目类型GUID
        /// </summary>
        private static string GetProjectTypeGuid(ProjectType projectType)
        {
            // Visual C++ 项目类型GUID
            return "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
        }
    }
}