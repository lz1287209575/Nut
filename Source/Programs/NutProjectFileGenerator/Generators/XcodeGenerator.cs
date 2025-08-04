using System.Text;
using NutProjectFileGenerator.Models;

namespace NutProjectFileGenerator.Generators
{
    /// <summary>
    /// Xcode项目文件生成器
    /// </summary>
    public class XcodeGenerator : ProjectFileGeneratorBase
    {
        public override string Name => "Xcode";

        public override string Description => "Generate Xcode project files (.xcodeproj)";

        public override List<string> SupportedPlatforms => new() { "MacOS" };

        private readonly Dictionary<string, string> fileUUIDs = new();
        private readonly Dictionary<string, string> targetUUIDs = new();
        private int uuidCounter = 0;

        public override bool CanGenerate()
        {
            return OperatingSystem.IsMacOS() || Environment.GetEnvironmentVariable("FORCE_XCODE_GEN") == "1";
        }

        public override async Task<bool> GenerateAsync(SolutionInfo solutionInfo, string outputDirectory)
        {
            try
            {
                var projectName = $"{solutionInfo.Name}.xcodeproj";
                var projectDir = Path.Combine(outputDirectory, projectName);
                EnsureDirectoryExists(projectDir);

                // 生成 project.pbxproj
                await GenerateProjectFileAsync(solutionInfo, projectDir);

                // 生成 xcschememanagement.plist
                await GenerateSchemeManagementAsync(solutionInfo, projectDir);

                // 为每个可执行项目生成scheme文件
                await GenerateSchemesAsync(solutionInfo, projectDir);

                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"生成Xcode项目失败: {ex.Message}");
                return false;
            }
        }

        public override List<string> GetGeneratedFiles(SolutionInfo solutionInfo, string outputDirectory)
        {
            var projectName = $"{solutionInfo.Name}.xcodeproj";
            var projectDir = Path.Combine(outputDirectory, projectName);
            var files = new List<string>
            {
                Path.Combine(projectDir, "project.pbxproj"),
                Path.Combine(projectDir, "xcshareddata", "xcschemes", "xcschememanagement.plist")
            };

            var executableProjects = solutionInfo.Projects.Where(p => p.Type == ProjectType.Executable);
            foreach (var project in executableProjects)
            {
                files.Add(Path.Combine(projectDir, "xcshareddata", "xcschemes", $"{project.Name}.xcscheme"));
            }

            return files;
        }

        /// <summary>
        /// 生成 project.pbxproj 文件
        /// </summary>
        private async Task GenerateProjectFileAsync(SolutionInfo solutionInfo, string projectDir)
        {
            var sb = new StringBuilder();

            // 初始化UUID映射
            foreach (var project in solutionInfo.Projects)
            {
                targetUUIDs[project.Name] = GenerateUUID();
                targetUUIDs[$"{project.Name}_Sources"] = GenerateUUID();
                targetUUIDs[$"{project.Name}_Headers"] = GenerateUUID();
            }

            sb.AppendLine("// !$*UTF8*$!");
            sb.AppendLine("{");
            sb.AppendLine("\tarchiveVersion = 1;");
            sb.AppendLine("\tclasses = {");
            sb.AppendLine("\t};");
            sb.AppendLine("\tobjectVersion = 56;");
            sb.AppendLine("\tobjects = {");
            sb.AppendLine();

            // PBXBuildFile section
            await GeneratePBXBuildFileSection(sb, solutionInfo);

            // PBXFileReference section
            await GeneratePBXFileReferenceSection(sb, solutionInfo);

            // PBXGroup section
            await GeneratePBXGroupSection(sb, solutionInfo);

            // PBXNativeTarget section
            await GeneratePBXNativeTargetSection(sb, solutionInfo);

            // PBXProject section
            await GeneratePBXProjectSection(sb, solutionInfo);

            // XCBuildConfiguration section
            await GenerateXCBuildConfigurationSection(sb, solutionInfo);

            // XCConfigurationList section
            await GenerateXCConfigurationListSection(sb, solutionInfo);

            sb.AppendLine("\t};");
            sb.AppendLine($"\trootObject = {GenerateUUID()} /* Project object */;");
            sb.AppendLine("}");

            await File.WriteAllTextAsync(Path.Combine(projectDir, "project.pbxproj"), sb.ToString());
        }

        /// <summary>
        /// 生成 PBXBuildFile 段
        /// </summary>
        private async Task GeneratePBXBuildFileSection(StringBuilder sb, SolutionInfo solutionInfo)
        {
            sb.AppendLine("/* Begin PBXBuildFile section */");
            
            foreach (var project in solutionInfo.Projects)
            {
                foreach (var sourceFile in project.SourceFiles)
                {
                    var fileUUID = GenerateUUID();
                    var fileName = Path.GetFileName(sourceFile);
                    sb.AppendLine($"\t\t{fileUUID} /* {fileName} in Sources */ = {{isa = PBXBuildFile; fileRef = {GenerateUUID()} /* {fileName} */; }};");
                }
            }
            
            sb.AppendLine("/* End PBXBuildFile section */");
            sb.AppendLine();
        }

        /// <summary>
        /// 生成 PBXFileReference 段
        /// </summary>
        private async Task GeneratePBXFileReferenceSection(StringBuilder sb, SolutionInfo solutionInfo)
        {
            sb.AppendLine("/* Begin PBXFileReference section */");
            
            foreach (var project in solutionInfo.Projects)
            {
                // 可执行文件引用
                if (project.Type == ProjectType.Executable)
                {
                    sb.AppendLine($"\t\t{GenerateUUID()} /* {project.OutputName} */ = {{isa = PBXFileReference; explicitFileType = \"compiled.mach-o.executable\"; includeInIndex = 0; path = {project.OutputName}; sourceTree = BUILT_PRODUCTS_DIR; }};");
                }
                else if (project.Type == ProjectType.StaticLibrary)
                {
                    sb.AppendLine($"\t\t{GenerateUUID()} /* lib{project.OutputName}.a */ = {{isa = PBXFileReference; explicitFileType = archive.ar; includeInIndex = 0; path = \"lib{project.OutputName}.a\"; sourceTree = BUILT_PRODUCTS_DIR; }};");
                }

                // 源文件引用
                foreach (var sourceFile in project.SourceFiles.Concat(project.HeaderFiles))
                {
                    var fileName = Path.GetFileName(sourceFile);
                    var relativePath = GetRelativePath(solutionInfo.SolutionRoot, sourceFile);
                    var fileType = GetXcodeFileType(sourceFile);
                    sb.AppendLine($"\t\t{GenerateUUID()} /* {fileName} */ = {{isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = {fileType}; path = \"{relativePath}\"; sourceTree = \"<group>\"; }};");
                }
            }
            
            sb.AppendLine("/* End PBXFileReference section */");
            sb.AppendLine();
        }

        /// <summary>
        /// 生成 PBXGroup 段
        /// </summary>
        private async Task GeneratePBXGroupSection(StringBuilder sb, SolutionInfo solutionInfo)
        {
            sb.AppendLine("/* Begin PBXGroup section */");
            
            // 主组
            sb.AppendLine($"\t\t{GenerateUUID()} = {{");
            sb.AppendLine("\t\t\tisa = PBXGroup;");
            sb.AppendLine("\t\t\tchildren = (");
            
            foreach (var project in solutionInfo.Projects)
            {
                sb.AppendLine($"\t\t\t\t{targetUUIDs[$"{project.Name}_Sources"]} /* {project.Name} */,");
            }
            
            sb.AppendLine($"\t\t\t\t{GenerateUUID()} /* Products */,");
            sb.AppendLine("\t\t\t);");
            sb.AppendLine("\t\t\tsourceTree = \"<group>\";");
            sb.AppendLine("\t\t};");

            // 每个项目的组
            foreach (var project in solutionInfo.Projects)
            {
                sb.AppendLine($"\t\t{targetUUIDs[$"{project.Name}_Sources"]} /* {project.Name} */ = {{");
                sb.AppendLine("\t\t\tisa = PBXGroup;");
                sb.AppendLine("\t\t\tchildren = (");
                
                foreach (var sourceFile in project.SourceFiles.Concat(project.HeaderFiles))
                {
                    var fileName = Path.GetFileName(sourceFile);
                    sb.AppendLine($"\t\t\t\t{GenerateUUID()} /* {fileName} */,");
                }
                
                sb.AppendLine("\t\t\t);");
                sb.AppendLine($"\t\t\tname = {project.Name};");
                sb.AppendLine("\t\t\tsourceTree = \"<group>\";");
                sb.AppendLine("\t\t};");
            }

            // Products 组
            sb.AppendLine($"\t\t{GenerateUUID()} /* Products */ = {{");
            sb.AppendLine("\t\t\tisa = PBXGroup;");
            sb.AppendLine("\t\t\tchildren = (");
            
            foreach (var project in solutionInfo.Projects)
            {
                var outputName = project.Type == ProjectType.StaticLibrary ? $"lib{project.OutputName}.a" : project.OutputName;
                sb.AppendLine($"\t\t\t\t{GenerateUUID()} /* {outputName} */,");
            }
            
            sb.AppendLine("\t\t\t);");
            sb.AppendLine("\t\t\tname = Products;");
            sb.AppendLine("\t\t\tsourceTree = \"<group>\";");
            sb.AppendLine("\t\t};");
            
            sb.AppendLine("/* End PBXGroup section */");
            sb.AppendLine();
        }

        /// <summary>
        /// 生成其他段（占位符实现）
        /// </summary>
        private async Task GeneratePBXNativeTargetSection(StringBuilder sb, SolutionInfo solutionInfo)
        {
            sb.AppendLine("/* Begin PBXNativeTarget section */");
            
            foreach (var project in solutionInfo.Projects)
            {
                var productType = project.Type == ProjectType.Executable 
                    ? "com.apple.product-type.tool" 
                    : "com.apple.product-type.library.static";
                
                sb.AppendLine($"\t\t{targetUUIDs[project.Name]} /* {project.Name} */ = {{");
                sb.AppendLine("\t\t\tisa = PBXNativeTarget;");
                sb.AppendLine($"\t\t\tname = \"{project.Name}\";");
                sb.AppendLine($"\t\t\tproductName = \"{project.Name}\";");
                sb.AppendLine($"\t\t\tproductType = \"{productType}\";");
                sb.AppendLine("\t\t};");
            }
            
            sb.AppendLine("/* End PBXNativeTarget section */");
            sb.AppendLine();
        }

        private async Task GeneratePBXProjectSection(StringBuilder sb, SolutionInfo solutionInfo)
        {
            sb.AppendLine("/* Begin PBXProject section */");
            sb.AppendLine($"\t\t{GenerateUUID()} /* Project object */ = {{");
            sb.AppendLine("\t\t\tisa = PBXProject;");
            sb.AppendLine("\t\t\tcompatibilityVersion = \"Xcode 14.0\";");
            sb.AppendLine("\t\t};");
            sb.AppendLine("/* End PBXProject section */");
            sb.AppendLine();
        }

        private async Task GenerateXCBuildConfigurationSection(StringBuilder sb, SolutionInfo solutionInfo)
        {
            sb.AppendLine("/* Begin XCBuildConfiguration section */");
            sb.AppendLine("/* End XCBuildConfiguration section */");
            sb.AppendLine();
        }

        private async Task GenerateXCConfigurationListSection(StringBuilder sb, SolutionInfo solutionInfo)
        {
            sb.AppendLine("/* Begin XCConfigurationList section */");
            sb.AppendLine("/* End XCConfigurationList section */");
            sb.AppendLine();
        }

        /// <summary>
        /// 生成scheme管理文件
        /// </summary>
        private async Task GenerateSchemeManagementAsync(SolutionInfo solutionInfo, string projectDir)
        {
            var sharedDataDir = Path.Combine(projectDir, "xcshareddata", "xcschemes");
            EnsureDirectoryExists(sharedDataDir);

            var plist = @"<?xml version=""1.0"" encoding=""UTF-8""?>
<!DOCTYPE plist PUBLIC ""-//Apple//DTD PLIST 1.0//EN"" ""http://www.apple.com/DTDs/PropertyList-1.0.dtd"">
<plist version=""1.0"">
<dict>
    <key>SchemeUserState</key>
    <dict>
    </dict>
    <key>SuppressBuildableAutocreation</key>
    <dict>
    </dict>
</dict>
</plist>";

            await File.WriteAllTextAsync(Path.Combine(sharedDataDir, "xcschememanagement.plist"), plist);
        }

        /// <summary>
        /// 生成scheme文件
        /// </summary>
        private async Task GenerateSchemesAsync(SolutionInfo solutionInfo, string projectDir)
        {
            var schemesDir = Path.Combine(projectDir, "xcshareddata", "xcschemes");
            EnsureDirectoryExists(schemesDir);

            var executableProjects = solutionInfo.Projects.Where(p => p.Type == ProjectType.Executable);
            foreach (var project in executableProjects)
            {
                var scheme = $@"<?xml version=""1.0"" encoding=""UTF-8""?>
<Scheme
   LastUpgradeVersion = ""1500""
   version = ""1.3"">
   <BuildAction
      parallelizeBuildables = ""YES""
      buildImplicitDependencies = ""YES"">
      <BuildActionEntries>
         <BuildActionEntry
            buildForTesting = ""YES""
            buildForRunning = ""YES""
            buildForProfiling = ""YES""
            buildForArchiving = ""YES""
            buildForAnalyzing = ""YES"">
         </BuildActionEntry>
      </BuildActionEntries>
   </BuildAction>
   <LaunchAction
      buildConfiguration = ""Debug""
      selectedDebuggerIdentifier = ""Xcode.DebuggerFoundation.Debugger.LLDB""
      selectedLauncherIdentifier = ""Xcode.DebuggerFoundation.Launcher.LLDB""
      launchStyle = ""0""
      useCustomWorkingDirectory = ""NO""
      ignoresPersistentStateOnLaunch = ""NO""
      debugDocumentVersioning = ""YES""
      debugServiceExtension = ""internal""
      allowLocationSimulation = ""YES"">
   </LaunchAction>
</Scheme>";

                await File.WriteAllTextAsync(Path.Combine(schemesDir, $"{project.Name}.xcscheme"), scheme);
            }
        }

        /// <summary>
        /// 生成UUID
        /// </summary>
        private string GenerateUUID()
        {
            return $"UUID{uuidCounter++:D8}00000000000000";
        }

        /// <summary>
        /// 获取Xcode文件类型
        /// </summary>
        private string GetXcodeFileType(string filePath)
        {
            var extension = Path.GetExtension(filePath).ToLower();
            return extension switch
            {
                ".cpp" or ".cc" or ".cxx" => "sourcecode.cpp.cpp",
                ".c" => "sourcecode.c.c",
                ".h" or ".hpp" or ".hxx" => "sourcecode.c.h",
                ".m" => "sourcecode.c.objc",
                ".mm" => "sourcecode.cpp.objcpp",
                _ => "text"
            };
        }
    }
}