using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using NutBuildTools.Utils;

namespace NutBuildTools.BuildSystem
{
    /// <summary>
    /// 编译器类型
    /// </summary>
    public enum CompilerType
    {
        GCC,
        Clang,
        MSVC
    }

    /// <summary>
    /// 编译器配置
    /// </summary>
    public class CompilerConfig
    {
        public CompilerType Type { get; set; }
        public string CompilerPath { get; set; } = string.Empty;
        public string LinkerPath { get; set; } = string.Empty;
        public string ArchiverPath { get; set; } = string.Empty;
        public List<string> DefaultFlags { get; set; } = new List<string>();
        public List<string> DebugFlags { get; set; } = new List<string>();
        public List<string> ReleaseFlags { get; set; } = new List<string>();
        public string ObjectExtension { get; set; } = ".o";
        public string ExecutableExtension { get; set; } = string.Empty;
        public string StaticLibExtension { get; set; } = ".a";

        /// <summary>
        /// 自动检测编译器
        /// </summary>
        public static CompilerConfig DetectCompiler(string platform)
        {
            Logger.Info($"检测编译器: 平台 {platform}");

            CompilerConfig config = new CompilerConfig();

            if (platform.Equals("Windows", StringComparison.OrdinalIgnoreCase))
            {
                config = DetectWindowsCompiler();
            }
            else if (platform.Equals("Mac", StringComparison.OrdinalIgnoreCase) || 
                     platform.Equals("macOS", StringComparison.OrdinalIgnoreCase))
            {
                config = DetectMacCompiler();
            }
            else if (platform.Equals("Linux", StringComparison.OrdinalIgnoreCase))
            {
                config = DetectLinuxCompiler();
            }
            else
            {
                // 根据当前运行平台检测
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                {
                    config = DetectWindowsCompiler();
                }
                else if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
                {
                    config = DetectMacCompiler();
                }
                else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    config = DetectLinuxCompiler();
                }
            }

            Logger.Info($"检测到编译器: {config.Type} at {config.CompilerPath}");
            return config;
        }

        private static CompilerConfig DetectWindowsCompiler()
        {
            // 优先检测 MSVC
            var msvcConfig = TryDetectMSVC();
            if (msvcConfig != null) return msvcConfig;

            // 然后检测 Clang
            var clangConfig = TryDetectClang();
            if (clangConfig != null)
            {
                SetupWindowsClangConfig(clangConfig);
                return clangConfig;
            }

            // 最后检测 GCC (MinGW)
            var gccConfig = TryDetectGCC();
            if (gccConfig != null)
            {
                SetupWindowsGccConfig(gccConfig);
                return gccConfig;
            }

            // 如果都没找到，尝试使用环境变量中的编译器
            Logger.Warning("未找到标准位置的编译器，尝试使用环境变量中的编译器");
            
            var fallbackClang = TryDetectClang();
            if (fallbackClang != null)
            {
                SetupWindowsClangConfig(fallbackClang);
                return fallbackClang;
            }

            throw new InvalidOperationException("未找到可用的 C++ 编译器。请确保安装了 Visual Studio、Clang 或 MinGW");
        }

        private static CompilerConfig DetectMacCompiler()
        {
            // macOS 优先使用 Clang (Xcode Command Line Tools)
            var clangConfig = TryDetectClang();
            if (clangConfig != null)
            {
                SetupMacClangConfig(clangConfig);
                return clangConfig;
            }

            // 备选 GCC (通过 Homebrew 安装)
            var gccConfig = TryDetectGCC();
            if (gccConfig != null)
            {
                SetupMacGccConfig(gccConfig);
                return gccConfig;
            }

            throw new InvalidOperationException("未找到可用的 C++ 编译器，请安装 Xcode Command Line Tools");
        }

        private static CompilerConfig DetectLinuxCompiler()
        {
            // Linux 优先使用 GCC
            var gccConfig = TryDetectGCC();
            if (gccConfig != null)
            {
                SetupLinuxGccConfig(gccConfig);
                return gccConfig;
            }

            // 备选 Clang
            var clangConfig = TryDetectClang();
            if (clangConfig != null)
            {
                SetupLinuxClangConfig(clangConfig);
                return clangConfig;
            }

            throw new InvalidOperationException("未找到可用的 C++ 编译器，请安装 GCC 或 Clang");
        }

        private static CompilerConfig? TryDetectMSVC()
        {
            try
            {
                // 检测 Visual Studio 2022/2019 的 MSVC
                var programFiles = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
                var programFilesX86 = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86);

                var possiblePaths = new[]
                {
                    Path.Combine(programFiles, "Microsoft Visual Studio", "2022", "Community", "VC", "Tools", "MSVC"),
                    Path.Combine(programFiles, "Microsoft Visual Studio", "2022", "Professional", "VC", "Tools", "MSVC"),
                    Path.Combine(programFiles, "Microsoft Visual Studio", "2022", "Enterprise", "VC", "Tools", "MSVC"),
                    Path.Combine(programFilesX86, "Microsoft Visual Studio", "2019", "Community", "VC", "Tools", "MSVC"),
                    Path.Combine(programFilesX86, "Microsoft Visual Studio", "2019", "Professional", "VC", "Tools", "MSVC"),
                    Path.Combine(programFilesX86, "Microsoft Visual Studio", "2019", "Enterprise", "VC", "Tools", "MSVC")
                };

                foreach (var basePath in possiblePaths)
                {
                    if (Directory.Exists(basePath))
                    {
                        var versions = Directory.GetDirectories(basePath);
                        if (versions.Length > 0)
                        {
                            var latestVersion = versions.OrderByDescending(x => x).First();
                            var compilerPath = Path.Combine(latestVersion, "bin", "Hostx64", "x64", "cl.exe");
                            var linkerPath = Path.Combine(latestVersion, "bin", "Hostx64", "x64", "link.exe");
                            var archiverPath = Path.Combine(latestVersion, "bin", "Hostx64", "x64", "lib.exe");

                            if (File.Exists(compilerPath))
                            {
                                return new CompilerConfig
                                {
                                    Type = CompilerType.MSVC,
                                    CompilerPath = compilerPath,
                                    LinkerPath = linkerPath,
                                    ArchiverPath = archiverPath,
                                    ObjectExtension = ".obj",
                                    ExecutableExtension = ".exe",
                                    StaticLibExtension = ".lib",
                                    DefaultFlags = new List<string> { "/std:c++20", "/EHsc", "/W3", "/utf-8" },
                                    DebugFlags = new List<string> { "/Od", "/Zi", "/MDd", "/DDEBUG" },
                                    ReleaseFlags = new List<string> { "/O2", "/MD", "/DNDEBUG" }
                                };
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.Warning($"检测 MSVC 失败: {ex.Message}");
            }

            return null;
        }

        private static CompilerConfig? TryDetectClang()
        {
            try
            {
                var clangPath = FindExecutableInPath("clang++");
                if (!string.IsNullOrEmpty(clangPath))
                {
                    return new CompilerConfig
                    {
                        Type = CompilerType.Clang,
                        CompilerPath = clangPath,
                        LinkerPath = clangPath, // Clang 可以直接用于链接
                        ArchiverPath = FindExecutableInPath("ar") ?? "ar"
                    };
                }
            }
            catch (Exception ex)
            {
                Logger.Warning($"检测 Clang 失败: {ex.Message}");
            }

            return null;
        }

        private static CompilerConfig? TryDetectGCC()
        {
            try
            {
                var gccPath = FindExecutableInPath("g++");
                if (!string.IsNullOrEmpty(gccPath))
                {
                    return new CompilerConfig
                    {
                        Type = CompilerType.GCC,
                        CompilerPath = gccPath,
                        LinkerPath = gccPath, // GCC 可以直接用于链接
                        ArchiverPath = FindExecutableInPath("ar") ?? "ar"
                    };
                }
            }
            catch (Exception ex)
            {
                Logger.Warning($"检测 GCC 失败: {ex.Message}");
            }

            return null;
        }

        private static string? FindExecutableInPath(string executable)
        {
            Logger.Debug($"正在搜索可执行文件: {executable}");
            
            var paths = Environment.GetEnvironmentVariable("PATH")?.Split(Path.PathSeparator) ?? Array.Empty<string>();
            var extensions = RuntimeInformation.IsOSPlatform(OSPlatform.Windows) 
                ? new[] { ".exe", ".cmd", ".bat" } 
                : new[] { "" };

            Logger.Debug($"PATH 环境变量包含 {paths.Length} 个路径");

            foreach (var path in paths)
            {
                foreach (var ext in extensions)
                {
                    var fullPath = Path.Combine(path, executable + ext);
                    Logger.Debug($"检查路径: {fullPath}");
                    
                    if (File.Exists(fullPath))
                    {
                        Logger.Debug($"找到可执行文件: {fullPath}");
                        return fullPath;
                    }
                }
            }

            Logger.Debug($"未找到可执行文件: {executable}");
            return null;
        }

        private static void SetupWindowsClangConfig(CompilerConfig config)
        {
            config.ExecutableExtension = ".exe";
            config.DefaultFlags = new List<string> { "-std=c++20", "-Wall", "-Wextra" };
            config.DebugFlags = new List<string> { "-g", "-O0", "-DDEBUG" };
            config.ReleaseFlags = new List<string> { "-O3", "-DNDEBUG" };
        }

        private static void SetupWindowsGccConfig(CompilerConfig config)
        {
            config.ExecutableExtension = ".exe";
            config.DefaultFlags = new List<string> { "-std=c++20", "-Wall", "-Wextra" };
            config.DebugFlags = new List<string> { "-g", "-O0", "-DDEBUG" };
            config.ReleaseFlags = new List<string> { "-O3", "-DNDEBUG" };
        }

        private static void SetupMacClangConfig(CompilerConfig config)
        {
            config.DefaultFlags = new List<string> { "-std=c++20", "-Wall", "-Wextra", "-mmacosx-version-min=10.15" };
            config.DebugFlags = new List<string> { "-g", "-O0", "-DDEBUG" };
            config.ReleaseFlags = new List<string> { "-O3", "-DNDEBUG" };
        }

        private static void SetupMacGccConfig(CompilerConfig config)
        {
            config.DefaultFlags = new List<string> { "-std=c++20", "-Wall", "-Wextra" };
            config.DebugFlags = new List<string> { "-g", "-O0", "-DDEBUG" };
            config.ReleaseFlags = new List<string> { "-O3", "-DNDEBUG" };
        }

        private static void SetupLinuxClangConfig(CompilerConfig config)
        {
            config.DefaultFlags = new List<string> { "-std=c++20", "-Wall", "-Wextra" };
            config.DebugFlags = new List<string> { "-g", "-O0", "-DDEBUG" };
            config.ReleaseFlags = new List<string> { "-O3", "-DNDEBUG" };
        }

        private static void SetupLinuxGccConfig(CompilerConfig config)
        {
            config.DefaultFlags = new List<string> { "-std=c++20", "-Wall", "-Wextra" };
            config.DebugFlags = new List<string> { "-g", "-O0", "-DDEBUG" };
            config.ReleaseFlags = new List<string> { "-O3", "-DNDEBUG" };
        }
    }
}