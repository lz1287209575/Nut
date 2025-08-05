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

        public override string Description => "Generate Visual Studio solution and project files (.sln, .vcxproj) compatible with Rider and other IDEs";

        public override List<string> SupportedPlatforms => new() { "Windows", "Linux", "MacOS" };

        private readonly Dictionary<string, string> projectGuids = new();
        private readonly Dictionary<string, string> solutionFolderGuids = new();
        private readonly string solutionGuid = Guid.NewGuid().ToString("B").ToUpper();

        public override bool CanGenerate()
        {
            // .sln 文件可以在任何平台上生成，因为它们是纯文本格式
            // Rider 和其他跨平台 IDE 都支持 .sln 文件
            return true;
        }

        public override async Task<bool> GenerateAsync(SolutionInfo solutionInfo, string outputDirectory)
        {
            try
            {
                // 根据平台过滤项目列表
                var filteredProjects = FilterProjectsByPlatform(solutionInfo.Projects);

                // 为每个项目生成GUID
                foreach (var project in filteredProjects)
                {
                    projectGuids[project.Name] = Guid.NewGuid().ToString("B").ToUpper();
                }

                // 为解决方案文件夹生成GUID
                var solutionFolders = GetSolutionFolders(filteredProjects, solutionInfo.SolutionRoot);
                foreach (var folderName in solutionFolders.Keys)
                {
                    solutionFolderGuids[folderName] = Guid.NewGuid().ToString("B").ToUpper();
                }

                // 生成解决方案文件
                await GenerateSolutionFileAsync(solutionInfo, outputDirectory, filteredProjects);

                // 为每个项目生成项目文件
                foreach (var project in filteredProjects)
                {
                    if (IsCSharpProject(project.Type))
                    {
                        await GenerateCSharpProjectFileAsync(project, solutionInfo, outputDirectory);
                    }
                    else
                    {
                        await GenerateProjectFileAsync(project, solutionInfo, outputDirectory);
                    }
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
                var projectFileName = GetProjectFileName(project);
                files.Add(Path.Combine(outputDirectory, projectFileName));
                
                // 只有C++项目需要.filters文件
                if (!IsCSharpProject(project.Type))
                {
                    files.Add(Path.Combine(outputDirectory, $"{project.Name}.vcxproj.filters"));
                }
            }

            return files;
        }

        /// <summary>
        /// 生成解决方案文件 (.sln)
        /// </summary>
        private async Task GenerateSolutionFileAsync(SolutionInfo solutionInfo, string outputDirectory, List<ProjectInfo> projects)
        {
            var sb = new StringBuilder();

            // 解决方案头部
            sb.AppendLine("Microsoft Visual Studio Solution File, Format Version 12.00");
            sb.AppendLine("# Visual Studio Version 17");
            sb.AppendLine("VisualStudioVersion = 17.0.31903.59");
            sb.AppendLine("MinimumVisualStudioVersion = 10.0.40219.1");

            // 生成解决方案文件夹
            var solutionFolders = GetSolutionFolders(projects, solutionInfo.SolutionRoot);
            var solutionFolderGuid = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}";

            foreach (var folderName in solutionFolders.Keys)
            {
                var folderGuid = solutionFolderGuids[folderName];
                sb.AppendLine($"Project(\"{solutionFolderGuid}\") = \"{folderName}\", \"{folderName}\", \"{folderGuid}\"");
                sb.AppendLine("EndProject");
            }

            // 项目声明
            foreach (var project in projects)
            {
                var projectTypeGuid = GetProjectTypeGuid(project.Type);
                var projectGuid = projectGuids[project.Name];
                var projectPath = GetProjectFilePath(project, solutionInfo.SolutionRoot, outputDirectory);

                sb.AppendLine($"Project(\"{projectTypeGuid}\") = \"{project.Name}\", \"{projectPath}\", \"{projectGuid}\"");
                sb.AppendLine("EndProject");
            }

            // Solution Items 文件夹（用于属性文件）
            var solutionItemsGuid = Guid.NewGuid().ToString("B").ToUpper();
            sb.AppendLine($"Project(\"{solutionFolderGuid}\") = \"Solution Items\", \"Solution Items\", \"{solutionItemsGuid}\"");
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
            foreach (var config in projects.SelectMany(p => p.SupportedConfigurations).Distinct())
            {
                sb.AppendLine($"\t\t{config}|x64 = {config}|x64");
                sb.AppendLine($"\t\t{config}|ARM64 = {config}|ARM64");
                if (OperatingSystem.IsWindows())
                {
                    sb.AppendLine($"\t\t{config}|Win32 = {config}|Win32");
                }
            }
            sb.AppendLine("\tEndGlobalSection");

            // 项目配置平台
            sb.AppendLine("\tGlobalSection(ProjectConfigurationPlatforms) = postSolution");
            foreach (var project in projects)
            {
                var projectGuid = projectGuids[project.Name];
                foreach (var config in project.SupportedConfigurations)
                {
                    if (IsCSharpProject(project.Type))
                    {
                        // C#项目使用AnyCPU配置
                        sb.AppendLine($"\t\t{projectGuid}.{config}|x64.ActiveCfg = {config}|AnyCPU");
                        sb.AppendLine($"\t\t{projectGuid}.{config}|x64.Build.0 = {config}|AnyCPU");
                        sb.AppendLine($"\t\t{projectGuid}.{config}|ARM64.ActiveCfg = {config}|AnyCPU");
                        sb.AppendLine($"\t\t{projectGuid}.{config}|ARM64.Build.0 = {config}|AnyCPU");
                        if (OperatingSystem.IsWindows())
                        {
                            sb.AppendLine($"\t\t{projectGuid}.{config}|Win32.ActiveCfg = {config}|AnyCPU");
                            sb.AppendLine($"\t\t{projectGuid}.{config}|Win32.Build.0 = {config}|AnyCPU");
                        }
                    }
                    else
                    {
                        // C++项目使用对应的平台配置
                        sb.AppendLine($"\t\t{projectGuid}.{config}|x64.ActiveCfg = {config}|x64");
                        sb.AppendLine($"\t\t{projectGuid}.{config}|x64.Build.0 = {config}|x64");
                        sb.AppendLine($"\t\t{projectGuid}.{config}|ARM64.ActiveCfg = {config}|ARM64");
                        sb.AppendLine($"\t\t{projectGuid}.{config}|ARM64.Build.0 = {config}|ARM64");
                        if (OperatingSystem.IsWindows())
                        {
                            sb.AppendLine($"\t\t{projectGuid}.{config}|Win32.ActiveCfg = {config}|Win32");
                            sb.AppendLine($"\t\t{projectGuid}.{config}|Win32.Build.0 = {config}|Win32");
                        }
                    }
                }
            }
            sb.AppendLine("\tEndGlobalSection");

            // 嵌套项目（解决方案文件夹层次结构）
            sb.AppendLine("\tGlobalSection(NestedProjects) = preSolution");
            foreach (var folder in solutionFolders)
            {
                var folderGuid = solutionFolderGuids[folder.Key];
                foreach (var project in folder.Value)
                {
                    var projectGuid = projectGuids[project.Name];
                    sb.AppendLine($"\t\t{projectGuid} = {folderGuid}");
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
                // x64 平台（所有系统）
                sb.AppendLine($"    <ProjectConfiguration Include=\"{config}|x64\">");
                sb.AppendLine($"      <Configuration>{config}</Configuration>");
                sb.AppendLine("      <Platform>x64</Platform>");
                sb.AppendLine("    </ProjectConfiguration>");
                
                // ARM64 平台（Mac M1/M2，ARM Linux）
                sb.AppendLine($"    <ProjectConfiguration Include=\"{config}|ARM64\">");
                sb.AppendLine($"      <Configuration>{config}</Configuration>");
                sb.AppendLine("      <Platform>ARM64</Platform>");
                sb.AppendLine("    </ProjectConfiguration>");
                
                // Win32 平台（仅 Windows）
                if (OperatingSystem.IsWindows())
                {
                    sb.AppendLine($"    <ProjectConfiguration Include=\"{config}|Win32\">");
                    sb.AppendLine($"      <Configuration>{config}</Configuration>");
                    sb.AppendLine("      <Platform>Win32</Platform>");
                    sb.AppendLine("    </ProjectConfiguration>");
                }
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
                var platforms = new List<string> { "x64", "ARM64" };
                if (OperatingSystem.IsWindows())
                {
                    platforms.Add("Win32");
                }

                foreach (var platform in platforms)
                {
                    sb.AppendLine($"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='{config}|{platform}'\" Label=\"Configuration\">");
                    
                    // 使用Makefile项目类型，让Visual Studio调用外部构建工具
                    var configurationType = "Makefile";
                    
                    sb.AppendLine($"    <ConfigurationType>{configurationType}</ConfigurationType>");
                    sb.AppendLine("    <UseDebugLibraries>" + (config == "Debug" ? "true" : "false") + "</UseDebugLibraries>");
                    
                    // 使用更通用的平台工具集设置
                    if (OperatingSystem.IsWindows())
                    {
                        sb.AppendLine("    <PlatformToolset>v143</PlatformToolset>");
                    }
                    else
                    {
                        // 为 Unix 系统提供更好的兼容性
                        sb.AppendLine("    <PlatformToolset>ClangCL</PlatformToolset>");
                    }
                    
                    sb.AppendLine("    <WholeProgramOptimization>" + (config == "Release" ? "true" : "false") + "</WholeProgramOptimization>");
                    sb.AppendLine("    <CharacterSet>Unicode</CharacterSet>");
                    
                    // 禁用标准构建过程，使用自定义构建
                    sb.AppendLine("    <NMakeBuildCommandLine>dotnet &quot;$(SolutionDir)Binary/NutBuildTools/NutBuildTools.dll&quot; build --target $(ProjectName) --configuration $(Configuration) --platform $(Platform)</NMakeBuildCommandLine>");
                    sb.AppendLine("    <NMakeCleanCommandLine>dotnet &quot;$(SolutionDir)Binary/NutBuildTools/NutBuildTools.dll&quot; build --target $(ProjectName) --configuration $(Configuration) --platform $(Platform) --clean</NMakeCleanCommandLine>");
                    sb.AppendLine("    <NMakeReBuildCommandLine>dotnet &quot;$(SolutionDir)Binary/NutBuildTools/NutBuildTools.dll&quot; build --target $(ProjectName) --configuration $(Configuration) --platform $(Platform) --rebuild</NMakeReBuildCommandLine>");
                    
                    // 为Makefile项目配置IntelliSense包含目录
                    var allIncludeDirs = project.IncludeDirectories.Concat(solutionInfo.GlobalIncludeDirectories).Distinct();
                    var includePathsForIntellisense = string.Join(";", allIncludeDirs.Select(dir => GetRelativePath(outputDirectory, dir)));
                    sb.AppendLine($"    <NMakeIncludeSearchPath>{includePathsForIntellisense}</NMakeIncludeSearchPath>");
                    
                    // 添加预处理器定义用于IntelliSense
                    var allDefines = project.PreprocessorDefinitions.Concat(solutionInfo.GlobalPreprocessorDefinitions).Distinct();
                    if (config == "Debug")
                    {
                        allDefines = allDefines.Concat(new[] { "_DEBUG" });
                    }
                    else
                    {
                        allDefines = allDefines.Concat(new[] { "NDEBUG" });
                    }
                    var definesForIntellisense = string.Join(";", allDefines);
                    sb.AppendLine($"    <NMakePreprocessorDefinitions>{definesForIntellisense}</NMakePreprocessorDefinitions>");
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
                var platforms = new List<string> { "x64", "ARM64" };
                if (OperatingSystem.IsWindows())
                {
                    platforms.Add("Win32");
                }

                foreach (var platform in platforms)
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
                var platforms = new List<string> { "x64", "ARM64" };
                if (OperatingSystem.IsWindows())
                {
                    platforms.Add("Win32");
                }

                foreach (var platform in platforms)
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
                var platforms = new List<string> { "x64", "ARM64" };
                if (OperatingSystem.IsWindows())
                {
                    platforms.Add("Win32");
                }

                foreach (var platform in platforms)
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

            // 重写构建目标使用NutBuildTools
            sb.AppendLine("  <!-- Override build targets to use NutBuildTools -->");
            sb.AppendLine("  <Target Name=\"CoreBuild\">");
            sb.AppendLine("    <Message Text=\"Building $(ProjectName) using NutBuildTools...\" Importance=\"high\" />");
            sb.AppendLine("    <Exec Command=\"dotnet &quot;$(SolutionDir)Binary/NutBuildTools/NutBuildTools.dll&quot; build --target $(ProjectName) --configuration $(Configuration) --platform $(Platform)\" ");
            sb.AppendLine("          WorkingDirectory=\"$(SolutionDir)\" ");
            sb.AppendLine("          ContinueOnError=\"false\" />");
            sb.AppendLine("  </Target>");
            sb.AppendLine("");
            sb.AppendLine("  <Target Name=\"CoreClean\">");
            sb.AppendLine("    <Message Text=\"Cleaning $(ProjectName) using NutBuildTools...\" Importance=\"high\" />");
            sb.AppendLine("    <Exec Command=\"dotnet &quot;$(SolutionDir)Binary/NutBuildTools/NutBuildTools.dll&quot; build --target $(ProjectName) --configuration $(Configuration) --platform $(Platform) --clean\" ");
            sb.AppendLine("          WorkingDirectory=\"$(SolutionDir)\" ");
            sb.AppendLine("          ContinueOnError=\"false\" />");
            sb.AppendLine("  </Target>");

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
                var filterPath = GetFilterPath(sourceFile);
                if (!string.IsNullOrEmpty(filterPath))
                {
                    var parts = filterPath.Split('\\', StringSplitOptions.RemoveEmptyEntries);
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
                var filterPath = GetFilterPath(headerFile);
                if (!string.IsNullOrEmpty(filterPath))
                {
                    var parts = filterPath.Split('\\', StringSplitOptions.RemoveEmptyEntries);
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
                    var filter = GetFilterPath(sourceFile);
                    
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
                    var filter = GetFilterPath(headerFile);
                    
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
            return projectType switch
            {
                ProjectType.CSharpLibrary => "{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}", // C# 类库
                ProjectType.CSharpExecutable => "{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}", // C# 可执行文件
                _ => "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}" // Visual C++ 项目
            };
        }

        /// <summary>
        /// 判断是否为C#项目
        /// </summary>
        private static bool IsCSharpProject(ProjectType projectType)
        {
            return projectType == ProjectType.CSharpLibrary || projectType == ProjectType.CSharpExecutable;
        }

        /// <summary>
        /// 获取项目文件名
        /// </summary>
        private static string GetProjectFileName(ProjectInfo project)
        {
            return IsCSharpProject(project.Type) ? $"{project.Name}.csproj" : $"{project.Name}.vcxproj";
        }

        /// <summary>
        /// 获取项目文件路径（用于.sln文件中的引用）
        /// </summary>
        private string GetProjectFilePath(ProjectInfo project, string solutionRoot, string outputDirectory)
        {
            if (IsCSharpProject(project.Type))
            {
                // 对于C#项目，尝试找到实际的项目文件
                var actualProjectPath = FindActualCSharpProject(project, solutionRoot);
                if (actualProjectPath != null)
                {
                    // 返回相对于输出目录的路径
                    return Path.GetRelativePath(outputDirectory, actualProjectPath).Replace('/', '\\');
                }
            }
            
            // 对于C++项目或未找到的C#项目，使用默认文件名
            return GetProjectFileName(project);
        }

        /// <summary>
        /// 生成C#项目文件
        /// </summary>
        private async Task GenerateCSharpProjectFileAsync(ProjectInfo project, SolutionInfo solutionInfo, string outputDirectory)
        {
            // 对于C#项目，我们创建一个简单的项目引用，指向实际的.csproj文件
            var actualProjectPath = FindActualCSharpProject(project, solutionInfo.SolutionRoot);
            if (actualProjectPath != null)
            {
                // 如果找到实际的.csproj文件，我们不需要生成新的，直接引用现有的
                // 但我们需要确保解决方案文件引用的是正确的相对路径
                return;
            }

            // 如果没有找到实际项目文件，生成一个基本的.csproj文件
            var sb = new StringBuilder();
            sb.AppendLine("<Project Sdk=\"Microsoft.NET.Sdk\">");
            sb.AppendLine("  <PropertyGroup>");
            sb.AppendLine("    <TargetFramework>net8.0</TargetFramework>");
            
            if (project.Type == ProjectType.CSharpExecutable)
            {
                sb.AppendLine("    <OutputType>Exe</OutputType>");
            }
            
            sb.AppendLine("  </PropertyGroup>");
            sb.AppendLine("</Project>");

            var projectPath = Path.Combine(outputDirectory, $"{project.Name}.csproj");
            await File.WriteAllTextAsync(projectPath, sb.ToString());
        }

        /// <summary>
        /// 查找实际的C#项目文件
        /// </summary>
        private string? FindActualCSharpProject(ProjectInfo project, string solutionRoot)
        {
            // 在解决方案根目录下搜索匹配的.csproj文件
            var csprojFiles = Directory.GetFiles(solutionRoot, $"{project.Name}.csproj", SearchOption.AllDirectories);
            return csprojFiles.FirstOrDefault();
        }

        /// <summary>
        /// 根据平台过滤项目列表
        /// </summary>
        private List<ProjectInfo> FilterProjectsByPlatform(List<ProjectInfo> projects)
        {
            // 在非Windows平台下，只保留C#项目
            if (!OperatingSystem.IsWindows())
            {
                return projects.Where(p => IsCSharpProject(p.Type)).ToList();
            }
            
            // Windows平台保留所有项目
            return projects;
        }

        /// <summary>
        /// 获取解决方案文件夹结构
        /// </summary>
        private Dictionary<string, List<ProjectInfo>> GetSolutionFolders(List<ProjectInfo> projects, string solutionRoot)
        {
            var solutionFolders = new Dictionary<string, List<ProjectInfo>>();

            foreach (var project in projects)
            {
                string folderName = DetermineSolutionFolder(project, solutionRoot);
                
                if (!solutionFolders.ContainsKey(folderName))
                {
                    solutionFolders[folderName] = new List<ProjectInfo>();
                }
                
                solutionFolders[folderName].Add(project);
            }

            return solutionFolders;
        }

        /// <summary>
        /// 基于Source目录结构确定项目应该放在哪个解决方案文件夹中
        /// </summary>
        private string DetermineSolutionFolder(ProjectInfo project, string solutionRoot)
        {
            // 对于C#项目，查找实际的项目文件路径
            if (IsCSharpProject(project.Type))
            {
                var actualProjectPath = FindActualCSharpProject(project, solutionRoot);
                if (actualProjectPath != null)
                {
                    return GetFolderFromPath(actualProjectPath, solutionRoot);
                }
            }

            // 对于C++项目，尝试通过项目名称在Source目录中找到对应的文件夹
            var sourceDir = Path.Combine(solutionRoot, "Source");
            if (Directory.Exists(sourceDir))
            {
                // 递归搜索包含项目名称的目录
                var projectDir = FindProjectDirectory(sourceDir, project.Name);
                if (projectDir != null)
                {
                    return GetFolderFromPath(projectDir, solutionRoot);
                }
            }

            // 如果找不到，返回默认文件夹
            return "Other";
        }

        /// <summary>
        /// 在指定目录中递归查找包含项目名称的目录
        /// </summary>
        private string? FindProjectDirectory(string searchDir, string projectName)
        {
            try
            {
                // 检查当前目录是否包含项目相关文件
                var buildFiles = Directory.GetFiles(searchDir, $"{projectName}.Build.cs", SearchOption.AllDirectories);
                if (buildFiles.Any())
                {
                    return Path.GetDirectoryName(buildFiles.First());
                }

                // 检查是否有同名目录
                var directories = Directory.GetDirectories(searchDir, "*", SearchOption.AllDirectories);
                var matchingDir = directories.FirstOrDefault(d => Path.GetFileName(d).Equals(projectName, StringComparison.OrdinalIgnoreCase));
                if (matchingDir != null)
                {
                    return matchingDir;
                }

                return null;
            }
            catch
            {
                return null;
            }
        }

        /// <summary>
        /// 获取筛选器路径，显示项目内部的相对路径（Sources、Meta、Config等）
        /// </summary>
        private string GetFilterPath(string filePath)
        {
            try
            {
                // 标准化路径分隔符
                var normalizedPath = filePath.Replace('/', '\\');
                
                // 查找常见的项目子目录（Sources、Meta、Config、Protos等）
                var projectDirs = new[] { "Sources", "Meta", "Config", "Protos", "Configs" };
                
                foreach (var dir in projectDirs)
                {
                    var dirPattern = $"\\{dir}\\";
                    var dirIndex = normalizedPath.LastIndexOf(dirPattern, StringComparison.OrdinalIgnoreCase);
                    if (dirIndex != -1)
                    {
                        // 找到目录，提取从该目录开始的路径
                        var startIndex = normalizedPath.IndexOf(dir, dirIndex, StringComparison.OrdinalIgnoreCase);
                        if (startIndex != -1)
                        {
                            var relativePath = normalizedPath.Substring(startIndex);
                            
                            // 获取目录部分，去掉文件名
                            var directoryPath = Path.GetDirectoryName(relativePath);
                            
                            // 如果就是在根目录（如Sources），返回该目录名
                            if (string.Equals(directoryPath, dir, StringComparison.OrdinalIgnoreCase))
                            {
                                return dir;
                            }
                            
                            return directoryPath ?? "";
                        }
                    }
                }
                
                // 如果没有找到标准项目目录，尝试查找文件直接在这些目录中的情况
                foreach (var dir in projectDirs)
                {
                    if (normalizedPath.EndsWith($"\\{dir}", StringComparison.OrdinalIgnoreCase) ||
                        normalizedPath.Contains($"\\{dir}\\"))
                    {
                        return dir;
                    }
                }
                
                // 都没找到，返回空字符串
                return "";
            }
            catch
            {
                return "";
            }
        }

        /// <summary>
        /// 从文件路径提取解决方案文件夹名称
        /// </summary>
        private string GetFolderFromPath(string fullPath, string solutionRoot)
        {
            try
            {
                var relativePath = Path.GetRelativePath(solutionRoot, fullPath);
                var pathParts = relativePath.Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
                
                // 跳过第一个"Source"部分，直接使用子目录名称
                if (pathParts.Length > 1 && pathParts[0] == "Source")
                {
                    // 只返回Source下的第一级目录名称
                    if (pathParts.Length >= 2)
                    {
                        return pathParts[1]; // 例如：Runtime, Programs
                    }
                }
                
                return "Other";
            }
            catch
            {
                return "Other";
            }
        }
    }
}