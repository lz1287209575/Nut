using System.Text.Json;
using NutProjectFileGenerator.Models;

namespace NutProjectFileGenerator.Generators
{
    /// <summary>
    /// VSCode配置文件生成器
    /// </summary>
    public class VSCodeGenerator : ProjectFileGeneratorBase
    {
        public override string Name => "VSCode";

        public override string Description => "Generate Visual Studio Code configuration files";

        public override List<string> SupportedPlatforms => new() { "Windows", "Linux", "MacOS" };

        public override async Task<bool> GenerateAsync(SolutionInfo solutionInfo, string outputDirectory)
        {
            try
            {
                var vscodeDir = Path.Combine(outputDirectory, ".vscode");
                EnsureDirectoryExists(vscodeDir);

                // 生成 c_cpp_properties.json
                await GenerateCppPropertiesAsync(solutionInfo, vscodeDir);

                // 生成 tasks.json
                await GenerateTasksAsync(solutionInfo, vscodeDir);

                // 生成 launch.json
                await GenerateLaunchAsync(solutionInfo, vscodeDir);

                // 生成 settings.json
                await GenerateSettingsAsync(solutionInfo, vscodeDir);

                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"生成VSCode配置失败: {ex.Message}");
                return false;
            }
        }

        public override List<string> GetGeneratedFiles(SolutionInfo solutionInfo, string outputDirectory)
        {
            var vscodeDir = Path.Combine(outputDirectory, ".vscode");
            return new List<string>
            {
                Path.Combine(vscodeDir, "c_cpp_properties.json"),
                Path.Combine(vscodeDir, "tasks.json"),
                Path.Combine(vscodeDir, "launch.json"),
                Path.Combine(vscodeDir, "settings.json")
            };
        }

        /// <summary>
        /// 生成 c_cpp_properties.json
        /// </summary>
        private async Task GenerateCppPropertiesAsync(SolutionInfo solutionInfo, string vscodeDir)
        {
            var includeDirectories = new HashSet<string>(solutionInfo.GlobalIncludeDirectories);
            
            // 收集所有项目的包含目录
            foreach (var project in solutionInfo.Projects)
            {
                foreach (var includeDir in project.IncludeDirectories)
                {
                    includeDirectories.Add(includeDir);
                }
            }

            var configurations = new object[]
            {
                    new
                    {
                        name = "Mac",
                        includePath = includeDirectories.Select(dir => NormalizePath(dir, '/')).ToArray(),
                        defines = solutionInfo.GlobalPreprocessorDefinitions.ToArray(),
                        macFrameworkPath = new[]
                        {
                            "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks"
                        },
                        compilerPath = "/usr/bin/clang++",
                        cStandard = "c17",
                        cppStandard = "c++20",
                        intelliSenseMode = "macos-clang-arm64"
                    },
                    new
                    {
                        name = "Linux",
                        includePath = includeDirectories.Select(dir => NormalizePath(dir, '/')).ToArray(),
                        defines = solutionInfo.GlobalPreprocessorDefinitions.ToArray(),
                        compilerPath = "/usr/bin/g++",
                        cStandard = "c17",
                        cppStandard = "c++20",
                        intelliSenseMode = "linux-gcc-x64"
                    },
                    new
                    {
                        name = "Win32",
                        includePath = includeDirectories.Select(dir => NormalizePath(dir, '\\')).ToArray(),
                        defines = solutionInfo.GlobalPreprocessorDefinitions.Concat(new[] { "_WIN32" }).ToArray(),
                        windowsSdkVersion = "10.0.22000.0",
                        compilerPath = "cl.exe",
                        cStandard = "c17",
                        cppStandard = "c++20",
                        intelliSenseMode = "windows-msvc-x64"
                    }
            };

            var cppProperties = new
            {
                version = 4,
                configurations = configurations
            };

            var json = JsonSerializer.Serialize(cppProperties, new JsonSerializerOptions 
            { 
                WriteIndented = true 
            });

            await File.WriteAllTextAsync(Path.Combine(vscodeDir, "c_cpp_properties.json"), json);
        }

        /// <summary>
        /// 生成 tasks.json
        /// </summary>
        private async Task GenerateTasksAsync(SolutionInfo solutionInfo, string vscodeDir)
        {
            var taskList = new object[]
            {
                    new
                    {
                        label = "Generate Project Files",
                        type = "shell",
                        command = "${workspaceFolder}/GenerateProjectFiles.sh",
                        group = "build",
                        presentation = new
                        {
                            echo = true,
                            reveal = "always",
                            focus = false,
                            panel = "shared"
                        },
                        problemMatcher = new string[0]
                    },
                    new
                    {
                        label = "Generate Headers",
                        type = "shell",
                        command = "${workspaceFolder}/Tools/Scripts/Unix/GenerateHeaders.sh",
                        args = new[] { "--verbose" },
                        group = "build",
                        presentation = new
                        {
                            echo = true,
                            reveal = "always",
                            focus = false,
                            panel = "shared"
                        },
                        problemMatcher = new string[0]
                    },
                    new
                    {
                        label = "Build Debug",
                        type = "shell",
                        command = "dotnet",
                        args = new[]
                        {
                            "run",
                            "--project",
                            "Source/Programs/NutBuildTools/NutBuildTools.csproj",
                            "--",
                            "build",
                            "--target",
                            "NutEngine",
                            "--platform",
                            "MacOS",
                            "--configuration",
                            "Debug"
                        },
                        group = new
                        {
                            kind = "build",
                            isDefault = true
                        },
                        presentation = new
                        {
                            echo = true,
                            reveal = "always",
                            focus = false,
                            panel = "shared"
                        },
                        problemMatcher = "$gcc"
                    },
                    new
                    {
                        label = "Build Release",
                        type = "shell",
                        command = "dotnet",
                        args = new[]
                        {
                            "run",
                            "--project",
                            "Source/Programs/NutBuildTools/NutBuildTools.csproj",
                            "--",
                            "build",
                            "--target",
                            "NutEngine",
                            "--platform",
                            "MacOS",
                            "--configuration",
                            "Release"
                        },
                        group = "build",
                        presentation = new
                        {
                            echo = true,
                            reveal = "always",
                            focus = false,
                            panel = "shared"
                        },
                        problemMatcher = "$gcc"
                    }
            };

            var tasks = new
            {
                version = "2.0.0",
                tasks = taskList
            };

            var json = JsonSerializer.Serialize(tasks, new JsonSerializerOptions { WriteIndented = true });
            await File.WriteAllTextAsync(Path.Combine(vscodeDir, "tasks.json"), json);
        }

        /// <summary>
        /// 生成 launch.json
        /// </summary>
        private async Task GenerateLaunchAsync(SolutionInfo solutionInfo, string vscodeDir)
        {
            var executableProjects = solutionInfo.Projects
                .Where(p => p.Type == ProjectType.Executable)
                .ToList();

            var configurations = new List<object>();

            foreach (var project in executableProjects)
            {
                configurations.Add(new
                {
                    name = $"Debug {project.Name}",
                    type = "cppdbg",
                    request = "launch",
                    program = $"${{workspaceFolder}}/Binary/{project.Name}/{project.OutputName}",
                    args = new string[0],
                    stopAtEntry = false,
                    cwd = "${workspaceFolder}",
                    environment = new object[0],
                    externalConsole = false,
                    MIMode = "lldb",
                    preLaunchTask = "Build Debug"
                });
            }

            var launch = new
            {
                version = "0.2.0",
                configurations = configurations.ToArray()
            };

            var json = JsonSerializer.Serialize(launch, new JsonSerializerOptions { WriteIndented = true });
            await File.WriteAllTextAsync(Path.Combine(vscodeDir, "launch.json"), json);
        }

        /// <summary>
        /// 生成 settings.json
        /// </summary>
        private async Task GenerateSettingsAsync(SolutionInfo solutionInfo, string vscodeDir)
        {
            var settings = new
            {
                files = new
                {
                    associations = new
                    {
                        __compat0 = "cpp",
                        __compat1 = "cpp"
                    },
                    exclude = new
                    {
                        __compat0 = true,
                        __compat1 = true,
                        __compat2 = true,
                        __compat3 = true,
                        __compat4 = true
                    }
                },
                search = new
                {
                    exclude = new
                    {
                        __compat0 = true,
                        __compat1 = true,
                        __compat2 = true
                    }
                }
            };

            var settingsDict = new Dictionary<string, object>
            {
                ["files.associations"] = new Dictionary<string, string>
                {
                    ["*.h"] = "cpp",
                    ["*.hpp"] = "cpp",
                    ["*.cpp"] = "cpp",
                    ["*.cc"] = "cpp",
                    ["*.cxx"] = "cpp",
                    ["*.generate.h"] = "cpp"
                },
                ["files.exclude"] = new Dictionary<string, bool>
                {
                    ["**/Binary"] = true,
                    ["**/Intermediate"] = true,
                    ["**/.DS_Store"] = true,
                    ["**/Thumbs.db"] = true,
                    ["**/*.xcworkspace"] = true,
                    ["**/*.xcodeproj"] = true,
                    ["**/*.sln"] = true,
                    ["**/*.vcxproj*"] = true
                },
                ["search.exclude"] = new Dictionary<string, bool>
                {
                    ["**/Binary"] = true,
                    ["**/Intermediate"] = true,
                    ["**/ThirdParty"] = true
                },
                ["C_Cpp.default.cppStandard"] = "c++20",
                ["C_Cpp.default.cStandard"] = "c17",
                ["C_Cpp.clang_format_style"] = "file",
                ["C_Cpp.autocompleteAddParentheses"] = true,
                ["C_Cpp.errorSquiggles"] = "enabled"
            };

            var json = JsonSerializer.Serialize(settingsDict, new JsonSerializerOptions { WriteIndented = true });
            await File.WriteAllTextAsync(Path.Combine(vscodeDir, "settings.json"), json);
        }
    }
}