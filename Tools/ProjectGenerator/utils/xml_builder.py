"""
XML 构建器

用于构建 XCode 项目文件和其他 XML 格式文件
"""

from typing import Dict, Any, List
from xml.dom.minidom import Document


class XmlBuilder:
    """XML 构建器"""
    
    def BuildPbxproj(self, project_data: Dict[str, Any]) -> str:
        """
        构建 XCode project.pbxproj 文件内容
        
        Args:
            project_data: 项目数据
            
        Returns:
            pbxproj 文件内容
        """
        lines = []
        
        # 文件头
        lines.extend([
            "// !$*UTF8*$!",
            "{",
            "\tarchiveVersion = 1;",
            "\tclasses = {",
            "\t};", 
            "\tobjectVersion = 46;",
            "\tobjects = {"
        ])
        
        uuids = project_data['uuids']
        
        # 添加 Project 对象
        lines.extend([
            f"\t\t{uuids['project']} /* Project object */ = {{",
            "\t\t\tisa = PBXProject;",
            "\t\t\tattributes = {",
            "\t\t\t\tLastUpgradeCheck = 9999;",
            "\t\t\t};",
            f"\t\t\tbuildConfigurationList = {uuids['config_list_project']};",
            "\t\t\tcompatibilityVersion = \"Xcode 3.2\";", 
            f"\t\t\tmainGroup = {uuids['main_group']};",
            "\t\t\ttargets = (",
            f"\t\t\t\t{uuids['target']},",
            "\t\t\t);",
            "\t\t};"
        ])
        
        # 添加主分组
        lines.extend([
            f"\t\t{uuids['main_group']} /* Main Group */ = {{",
            "\t\t\tisa = PBXGroup;",
            "\t\t\tchildren = ("
        ])
        
        for child in project_data['main_group_children']:
            lines.append(f"\t\t\t\t{child['uuid']} /* {child['name']} */,")
        
        # 为 Products 组生成 UUID
        products_group_uuid = "1234567890ABCDEF12345678"  # 临时使用固定 UUID
        lines.append(f"\t\t\t\t{products_group_uuid} /* Products */,")
        
        lines.extend([
            "\t\t\t);",
            "\t\t\tsourceTree = \"<group>\";",
            "\t\t};"
        ])
        
        # 添加 Products 分组
        lines.extend([
            f"\t\t{products_group_uuid} /* Products */ = {{",
            "\t\t\tisa = PBXGroup;",
            "\t\t\tchildren = (",
            f"\t\t\t\t{uuids['product_ref']} /* {project_data['project_name']} */,",
            "\t\t\t);",
            "\t\t\tname = Products;",
            "\t\t\tsourceTree = \"<group>\";",
            "\t\t};"
        ])
        
        # 添加文件分组
        for group in project_data['groups']:
            lines.extend([
                f"\t\t{group['uuid']} /* {group['name']} */ = {{",
                "\t\t\tisa = PBXGroup;",
                "\t\t\tchildren = ("
            ])
            
            for child in group['children']:
                lines.append(f"\t\t\t\t{child['uuid']} /* {child['name']} */,")
            
            lines.extend([
                "\t\t\t);",
                f"\t\t\tname = \"{group['name']}\";",
                "\t\t\tsourceTree = \"<group>\";",
                "\t\t};"
            ])
        
        # 添加 Target（包含 Sources Build Phase 用于 IntelliSense 和 NutBuild 用于实际编译）
        lines.extend([
            f"\t\t{uuids['target']} /* {project_data['project_name']} */ = {{",
            "\t\t\tisa = PBXNativeTarget;",
            f"\t\t\tname = \"{project_data['project_name']}\";",
            f"\t\t\tproductType = \"{project_data['product_type']}\";",
            f"\t\t\tbuildConfigurationList = {uuids['config_list_target']};",
            "\t\t\tbuildPhases = (",
            f"\t\t\t\t{uuids['build_phase_sources']},",
            f"\t\t\t\t{uuids['build_phase_nutbuild']},",
            "\t\t\t);",
            "\t\t\tbuildRules = (",
            "\t\t\t);",
            "\t\t\tdependencies = (",
            "\t\t\t);",
            f"\t\t\tproductReference = {uuids['product_ref']};",
            "\t\t};"
        ])
        
        # 添加文件引用
        for file_ref in project_data['file_refs']:
            lines.extend([
                f"\t\t{file_ref['uuid']} /* {file_ref['name']} */ = {{",
                "\t\t\tisa = PBXFileReference;",
                f"\t\t\tlastKnownFileType = {file_ref['file_type']};",
                f"\t\t\tpath = \"{file_ref['path']}\";",
                f"\t\t\tname = \"{file_ref['name']}\";",
                f"\t\t\tsourceTree = \"{file_ref['source_tree']}\";",
                "\t\t};"
            ])
        
        # 添加产品引用
        product_extension = ".a" if project_data['product_type'] == "com.apple.product-type.library.static" else ""
        explicit_file_type = "archive.ar" if project_data['product_type'] == "com.apple.product-type.library.static" else "compiled.mach-o.executable"
        
        lines.extend([
            f"\t\t{uuids['product_ref']} /* {project_data['project_name']}{product_extension} */ = {{",
            "\t\t\tisa = PBXFileReference;",
            f"\t\t\texplicitFileType = \"{explicit_file_type}\";",
            f"\t\t\tpath = \"{project_data['project_name']}{product_extension}\";",
            "\t\t\tsourceTree = \"BUILT_PRODUCTS_DIR\";",
            "\t\t};"
        ])
        
        # 添加构建文件（用于 Sources Build Phase 的 IntelliSense 分析）
        for build_file in project_data['build_files']:
            lines.extend([
                f"\t\t{build_file['uuid']} /* {build_file['file_name']} in Sources */ = {{",
                "\t\t\tisa = PBXBuildFile;",
                f"\t\t\tfileRef = {build_file['file_ref']};",
                "\t\t};"
            ])
        
        # 添加 NutBuild Shell Script Build Phase
        shell_script = self._GenerateXcodeNutBuildScript(project_data['project_name'])
        
        # 根据项目类型确定输出文件
        if project_data['product_type'] == "com.apple.product-type.library.static":
            output_file = f"$(SRCROOT)/../../Binary/{project_data['project_name']}.a"
        else:
            output_file = f"$(SRCROOT)/../../Binary/{project_data['project_name']}"
        
        # 准备输入文件列表（源文件和元数据文件）
        input_paths = []
        for file_ref in project_data['file_refs']:
            if file_ref['path'].endswith(('.cpp', '.c', '.cc', '.cxx', '.h', '.hpp', '.cs')):
                input_paths.append(f"\t\t\t\t\"$(SRCROOT)/{file_ref['path']}\",")
        
        lines.extend([
            f"\t\t{uuids['build_phase_nutbuild']} /* NutBuild */ = {{",
            "\t\t\tisa = PBXShellScriptBuildPhase;",
            "\t\t\tbuildActionMask = 2147483647;",
            "\t\t\tfiles = (",
            "\t\t\t);",
            "\t\t\tinputFileListPaths = (",
            "\t\t\t);",
            "\t\t\tinputPaths = (",
        ])
        
        # 添加输入文件路径
        lines.extend(input_paths)
        
        lines.extend([
            "\t\t\t);",
            "\t\t\tname = \"NutBuild\";",
            "\t\t\toutputFileListPaths = (",
            "\t\t\t);",
            "\t\t\toutputPaths = (",
            f"\t\t\t\t\"{output_file}\",",
            "\t\t\t);",
            "\t\t\trunOnlyForDeploymentPostprocessing = 0;",
            "\t\t\tshellPath = /bin/bash;",
            f"\t\t\tshellScript = \"{shell_script}\";",
            "\t\t\tshowEnvVarsInLog = 1;",
            "\t\t};"
        ])
        
        # 添加 Sources Build Phase（用于 IntelliSense，配置为不执行实际编译）
        lines.extend([
            f"\t\t{uuids['build_phase_sources']} /* Sources */ = {{",
            "\t\t\tisa = PBXSourcesBuildPhase;",
            "\t\t\tbuildActionMask = 0;",  # 设置为 0 禁用实际构建
            "\t\t\tfiles = ("
        ])
        
        for build_file in project_data['build_files']:
            lines.append(f"\t\t\t\t{build_file['uuid']} /* {build_file['file_name']} in Sources */,")
        
        lines.extend([
            "\t\t\t);",
            "\t\t\trunOnlyForDeploymentPostprocessing = 1;",  # 设置为 1 跳过构建
            "\t\t};"
        ])
        
        # 添加构建配置
        self._AddBuildConfigurations(lines, uuids, project_data['project_name'])
        
        # 添加配置列表
        self._AddConfigurationLists(lines, uuids)
        
        # 文件尾
        lines.extend([
            "\t};",
            f"\trootObject = {uuids['project']};",
            "}"
        ])
        
        return '\n'.join(lines)
    
    def _GenerateXcodeNutBuildScript(self, project_name: str) -> str:
        """生成优化的 Xcode NutBuild 脚本，提供更好的输出显示"""
        script_lines = [
            "#!/bin/bash",
            "set -e  # Exit on error",
            "",
            "# === NutBuild for Xcode ===",
            "echo \"🔨 Building project: {}\"".format(project_name),
            "echo \"📁 Xcode SRCROOT: $SRCROOT\"",
            "echo \"⚙️  Configuration: $CONFIGURATION\"",
            "echo \"🖥️  Platform: $PLATFORM_NAME\"",
            "echo \"\"",
            "",
            "# Find project root (contains CLAUDE.md)",
            "PROJECT_ROOT=\"$SRCROOT\"",
            "while [ ! -f \"$PROJECT_ROOT/CLAUDE.md\" ] && [ \"$PROJECT_ROOT\" != \"/\" ]; do",
            "    PROJECT_ROOT=\"$(dirname \"$PROJECT_ROOT\")\"",
            "done",
            "",
            "if [ ! -f \"$PROJECT_ROOT/CLAUDE.md\" ]; then",
            "    echo \"❌ Error: Could not find project root (CLAUDE.md not found)\"",
            "    exit 1",
            "fi",
            "",
            "echo \"✅ Found project root: $PROJECT_ROOT\"",
            "cd \"$PROJECT_ROOT\"",
            "echo \"\"",
            "",
            "# Setup NutBuildTools",
            "NUTBUILD_BINARY=\"$PROJECT_ROOT/Binary/NutBuildTools/NutBuildTools\"",
            "",
            "if [ ! -f \"$NUTBUILD_BINARY\" ]; then",
            "    echo \"📦 NutBuildTools binary not found, building...\"",
            "    echo \"\"",
            "    ",
            "    # Find dotnet",
            "    DOTNET_PATH=\"\"",
            "    if [ -f \"/usr/local/share/dotnet/dotnet\" ]; then",
            "        DOTNET_PATH=\"/usr/local/share/dotnet/dotnet\"",
            "    elif [ -f \"/opt/homebrew/bin/dotnet\" ]; then",
            "        DOTNET_PATH=\"/opt/homebrew/bin/dotnet\"",
            "    elif command -v dotnet >/dev/null 2>&1; then",
            "        DOTNET_PATH=\"dotnet\"",
            "    else",
            "        echo \"❌ Error: dotnet not found\"",
            "        echo \"💡 Please install .NET SDK from https://dotnet.microsoft.com/download\"", 
            "        exit 1",
            "    fi",
            "    ",
            "    echo \"🔧 Using dotnet at: $DOTNET_PATH\"",
            "    echo \"📦 Building NutBuildTools...\"",
            "    echo \"\"",
            "    ",
            "    # Build with output",
            "    \"$DOTNET_PATH\" publish Source/Programs/NutBuildTools -c Release -o Binary/NutBuildTools",
            "    ",
            "    if [ ! -f \"$NUTBUILD_BINARY\" ]; then",
            "        echo \"❌ Error: Failed to build NutBuildTools\"",
            "        exit 1",
            "    fi",
            "    echo \"\"",
            "    echo \"✅ NutBuildTools built successfully\"",
            "else",
            "    echo \"✅ NutBuildTools binary found\"",
            "fi",
            "",
            "echo \"\"",
            "echo \"🚀 Starting compilation with NutBuildTools...\"",
            "echo \"\"",
            "",
            "# Run NutBuildTools with Mac platform (Darwin internal name)",
            "\"$NUTBUILD_BINARY\" --target {} --platform Darwin --configuration \"$CONFIGURATION\"".format(project_name),
            "",
            "BUILD_RESULT=$?",
            "echo \"\"",
            "if [ $BUILD_RESULT -eq 0 ]; then",
            "    echo \"✅ Build completed successfully!\"",
            "else",
            "    echo \"❌ Build failed with exit code $BUILD_RESULT\"",
            "    exit $BUILD_RESULT",
            "fi"
        ]
        
        # Join lines and escape properly for pbxproj format
        script_content = "\\n".join(script_lines)
        # Escape quotes and backslashes for pbxproj format
        script_content = script_content.replace("\\", "\\\\").replace("\"", "\\\"")
        return script_content
    
    def _AddBuildConfigurations(self, lines: List[str], uuids: Dict[str, str], project_name: str):
        """添加构建配置"""
        # 项目级配置 - Debug
        lines.extend([
            f"\t\t{uuids['config_debug_project']} /* Debug */ = {{",
            "\t\t\tisa = XCBuildConfiguration;",
            "\t\t\tbuildSettings = {",
            "\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;",
            "\t\t\t\tCLANG_ANALYZER_NONNULL = YES;",
            "\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"gnu++20\";",
            "\t\t\t\tCLANG_ENABLE_MODULES = YES;",
            "\t\t\t\tCLANG_WARN_BOOL_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_CONSTANT_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_EMPTY_BODY = YES;",
            "\t\t\t\tCLANG_WARN_ENUM_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_INT_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_UNREACHABLE_CODE = YES;",
            "\t\t\t\tCOPY_PHASE_STRIP = NO;",
            "\t\t\t\tDEBUG_INFORMATION_FORMAT = dwarf;",
            "\t\t\t\tENABLE_STRICT_OBJC_MSGSEND = YES;",
            "\t\t\t\tENABLE_TESTABILITY = YES;",
            "\t\t\t\tGCC_C_LANGUAGE_STANDARD = gnu11;",
            "\t\t\t\tGCC_DYNAMIC_NO_PIC = NO;",
            "\t\t\t\tGCC_NO_COMMON_BLOCKS = YES;",
            "\t\t\t\tGCC_OPTIMIZATION_LEVEL = 0;",
            "\t\t\t\tGCC_PREPROCESSOR_DEFINITIONS = (",
            "\t\t\t\t\t\"DEBUG=1\",",
            "\t\t\t\t\t\"$(inherited)\",",
            "\t\t\t\t);",
            "\t\t\t\tMACOSX_DEPLOYMENT_TARGET = 10.15;",
            "\t\t\t\tONLY_ACTIVE_ARCH = YES;",
            "\t\t\t\tSDKROOT = macosx;",
            "\t\t\t};",
            "\t\t\tname = Debug;",
            "\t\t};",
        ])
        
        # 项目级配置 - Release
        lines.extend([
            f"\t\t{uuids['config_release_project']} /* Release */ = {{",
            "\t\t\tisa = XCBuildConfiguration;",
            "\t\t\tbuildSettings = {",
            "\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;",
            "\t\t\t\tCLANG_ANALYZER_NONNULL = YES;",
            "\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"gnu++20\";",
            "\t\t\t\tCLANG_ENABLE_MODULES = YES;",
            "\t\t\t\tCLANG_WARN_BOOL_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_CONSTANT_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_EMPTY_BODY = YES;",
            "\t\t\t\tCLANG_WARN_ENUM_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_INT_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_UNREACHABLE_CODE = YES;",
            "\t\t\t\tCOPY_PHASE_STRIP = NO;",
            "\t\t\t\tDEBUG_INFORMATION_FORMAT = \"dwarf-with-dsym\";",
            "\t\t\t\tENABLE_NS_ASSERTIONS = NO;",
            "\t\t\t\tENABLE_STRICT_OBJC_MSGSEND = YES;",
            "\t\t\t\tGCC_C_LANGUAGE_STANDARD = gnu11;",
            "\t\t\t\tGCC_NO_COMMON_BLOCKS = YES;",
            "\t\t\t\tMACOSX_DEPLOYMENT_TARGET = 10.15;",
            "\t\t\t\tSDKROOT = macosx;",
            "\t\t\t};",
            "\t\t\tname = Release;",
            "\t\t};"
        ])
        
        # Target 级配置 - Debug
        lines.extend([
            f"\t\t{uuids['config_debug_target']} /* Debug */ = {{",
            "\t\t\tisa = XCBuildConfiguration;",
            "\t\t\tbuildSettings = {",
            "\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";",
            "\t\t\t\t// 搜索路径配置",
            "\t\t\t\tUSER_HEADER_SEARCH_PATHS = \"$(SRCROOT)/../../Source/**\";",
            "\t\t\t\tHEADER_SEARCH_PATHS = \"$(SRCROOT)/../../ThirdParty/**\";",
            "\t\t\t\t// C++ 语言标准",
            "\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"gnu++20\";",
            "\t\t\t\tCLANG_CXX_LIBRARY = \"libc++\";",
            "\t\t\t\t// 启用代码分析但跳过实际编译",
            "\t\t\t\tSKIP_INSTALL = YES;",
            "\t\t\t\tCODE_SIGN_IDENTITY = \"\";",
            "\t\t\t\t// 禁用原生构建但保留 IntelliSense",
            "\t\t\t\tBUILD_ACTIVE_ARCHITECTURE_ONLY = NO;",
            "\t\t\t\tCOMPILE_SOURCES_BUILD_PHASE_ENABLED = NO;",
            "\t\t\t\tRUN_ONLY_FOR_DEPLOYMENT_POSTPROCESSING = YES;",
            "\t\t\t\t// 强制禁用编译器调用",
            "\t\t\t\tGCC_PREPROCESSOR_DEFINITIONS = (",
            "\t\t\t\t\t\"XCODE_INTELLISENSE_ONLY=1\",",
            "\t\t\t\t\t\"$(inherited)\",",
            "\t\t\t\t);",
            "\t\t\t\tOTHER_CFLAGS = \"-fsyntax-only\";",
            "\t\t\t\t// IntelliSense 相关设置",
            "\t\t\t\tCLANG_ANALYZER_NONNULL = YES;",
            "\t\t\t\tCLANG_WARN_BOOL_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_CONSTANT_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_EMPTY_BODY = YES;",
            "\t\t\t\tCLANG_WARN_ENUM_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_INT_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_UNREACHABLE_CODE = YES;",
            "\t\t\t\tGCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;",
            "\t\t\t\tGCC_WARN_UNDECLARED_SELECTOR = YES;",
            "\t\t\t\tGCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;",
            "\t\t\t\tGCC_WARN_UNUSED_FUNCTION = YES;",
            "\t\t\t\tGCC_WARN_UNUSED_VARIABLE = YES;",
            "\t\t\t};",
            "\t\t\tname = Debug;",
            "\t\t};",
        ])
        
        # Target 级配置 - Release
        lines.extend([
            f"\t\t{uuids['config_release_target']} /* Release */ = {{",
            "\t\t\tisa = XCBuildConfiguration;",
            "\t\t\tbuildSettings = {",
            "\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";",
            "\t\t\t\t// 搜索路径配置",
            "\t\t\t\tUSER_HEADER_SEARCH_PATHS = \"$(SRCROOT)/../../Source/**\";",
            "\t\t\t\tHEADER_SEARCH_PATHS = \"$(SRCROOT)/../../ThirdParty/**\";",
            "\t\t\t\t// C++ 语言标准",
            "\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"gnu++20\";",
            "\t\t\t\tCLANG_CXX_LIBRARY = \"libc++\";",
            "\t\t\t\t// 启用代码分析但跳过实际编译",
            "\t\t\t\tSKIP_INSTALL = YES;",
            "\t\t\t\tCODE_SIGN_IDENTITY = \"\";",
            "\t\t\t\t// 禁用原生构建但保留 IntelliSense",
            "\t\t\t\tBUILD_ACTIVE_ARCHITECTURE_ONLY = NO;",
            "\t\t\t\tCOMPILE_SOURCES_BUILD_PHASE_ENABLED = NO;",
            "\t\t\t\tRUN_ONLY_FOR_DEPLOYMENT_POSTPROCESSING = YES;",
            "\t\t\t\t// 强制禁用编译器调用",
            "\t\t\t\tGCC_PREPROCESSOR_DEFINITIONS = (",
            "\t\t\t\t\t\"XCODE_INTELLISENSE_ONLY=1\",",
            "\t\t\t\t\t\"$(inherited)\",",
            "\t\t\t\t);",
            "\t\t\t\tOTHER_CFLAGS = \"-fsyntax-only\";",
            "\t\t\t\t// IntelliSense 相关设置",
            "\t\t\t\tCLANG_ANALYZER_NONNULL = YES;",
            "\t\t\t\tCLANG_WARN_BOOL_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_CONSTANT_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_EMPTY_BODY = YES;",
            "\t\t\t\tCLANG_WARN_ENUM_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_INT_CONVERSION = YES;",
            "\t\t\t\tCLANG_WARN_UNREACHABLE_CODE = YES;",
            "\t\t\t\tGCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;",
            "\t\t\t\tGCC_WARN_UNDECLARED_SELECTOR = YES;",
            "\t\t\t\tGCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;",
            "\t\t\t\tGCC_WARN_UNUSED_FUNCTION = YES;",
            "\t\t\t\tGCC_WARN_UNUSED_VARIABLE = YES;",
            "\t\t\t};",
            "\t\t\tname = Release;",
            "\t\t};"
        ])
    
    def _AddConfigurationLists(self, lines: List[str], uuids: Dict[str, str]):
        """添加配置列表"""
        lines.extend([
            f"\t\t{uuids['config_list_project']} /* Build configuration list for PBXProject */ = {{",
            "\t\t\tisa = XCConfigurationList;",
            "\t\t\tbuildConfigurations = (",
            f"\t\t\t\t{uuids['config_debug_project']},",
            f"\t\t\t\t{uuids['config_release_project']},",
            "\t\t\t);",
            "\t\t\tdefaultConfigurationIsVisible = 0;",
            "\t\t\tdefaultConfigurationName = Release;",
            "\t\t};",
            f"\t\t{uuids['config_list_target']} /* Build configuration list for PBXNativeTarget */ = {{",
            "\t\t\tisa = XCConfigurationList;",
            "\t\t\tbuildConfigurations = (",
            f"\t\t\t\t{uuids['config_debug_target']},",
            f"\t\t\t\t{uuids['config_release_target']},",
            "\t\t\t);",
            "\t\t\tdefaultConfigurationIsVisible = 0;",
            "\t\t\tdefaultConfigurationName = Release;",
            "\t\t};"
        ])