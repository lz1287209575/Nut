#!/bin/bash

# Nut 项目文件生成器 (Shell 版本)
# 用于在 Setup 时创建基础项目文件，在 GenerateProjectFiles 时更新项目列表

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# 生成 MD5 哈希作为 GUID
generate_guid() {
    local name="$1"
    echo -n "$name" | md5sum | awk '{print $1}' | sed 's/\([a-f0-9]\{8\}\)\([a-f0-9]\{4\}\)\([a-f0-9]\{4\}\)\([a-f0-9]\{4\}\)\([a-f0-9]\{12\}\)/{\1-\2-\3-\4-\5}/'
}

# 获取相对路径 (兼容 macOS)
get_relative_path() {
    local target="$1"
    local base="$2"

    # 在 macOS 上使用不同的方法
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS 版本
        python3 -c "
import os
import sys
target = '$target'
base = '$base'
try:
    rel_path = os.path.relpath(target, base)
    print(rel_path)
except ValueError:
    print(target)
"
    else
        # Linux 版本
        realpath --relative-to="$base" "$target"
    fi
}

# 获取项目分组
get_project_group() {
    local project_path="$1"
    local source_dir="$PROJECT_ROOT/Source"

    # 获取相对于 Source 的路径
    local relative_to_source=$(get_relative_path "$project_path" "$source_dir")

    # 提取第一级目录名
    local group=$(echo "$relative_to_source" | cut -d'/' -f1)

    # 如果没有分组，使用 "Other"
    if [ -z "$group" ] || [ "$group" = "." ]; then
        group="Other"
    fi

    echo "$group"
}

# 检查分组是否存在
group_exists() {
    local group="$1"
    local groups="$2"
    echo "$groups" | grep -q "|$group|"
}

# 添加分组
add_group() {
    local group="$1"
    local groups="$2"
    if ! group_exists "$group" "$groups"; then
        echo "$groups|$group|"
    else
        echo "$groups"
    fi
}

# 发现项目
discover_projects() {
    local projects=()

    echo "🔍 正在发现项目..."

    # 扫描 Source 目录下的所有子目录
    while IFS= read -r -d '' dir; do
        local relative_path=$(get_relative_path "$dir" "$PROJECT_ROOT")
        local dir_name=$(basename "$dir")

        # 检查是否为 C# 项目（包含 .csproj 文件）
        if find "$dir" -name "*.csproj" -type f | grep -q .; then
            local csproj_file=$(find "$dir" -name "*.csproj" -type f | head -1)
            local project_name=$(basename "$csproj_file" .csproj)
            local group=$(get_project_group "$dir")
            projects+=("csharp|$project_name|$relative_path|$group")
            echo "  📂 $group:"
            echo "    - $project_name (csharp)"
        # 检查是否为 C++ 项目（包含 .Build.py 文件）
        elif find "$dir" -name "*.Build.cs" -type f | grep -q .; then
            local build_file=$(find "$dir" -name "*.Build.cs" -type f | head -1)
            local project_name=$(basename "$build_file" .Build.py)
            local group=$(get_project_group "$dir")
            projects+=("cpp|$project_name|$relative_path|$group")
            echo "  📂 $group:"
            echo "    - $project_name (cpp)"
        fi
    done < <(find "$PROJECT_ROOT/Source" -type d -mindepth 1 -maxdepth 2 -print0)

    echo ""
    echo "📁 发现 ${#projects[@]} 个项目:"

    # 按分组显示项目
    local current_group=""
    for project in "${projects[@]}"; do
        IFS='|' read -r type name path group <<<"$project"
        if [ "$group" != "$current_group" ]; then
            echo "  📂 $group:"
            current_group="$group"
        fi
        echo "    - $name ($type)"
    done

    echo ""
    echo "${projects[@]}"
}

# 生成 Xcode 项目文件 (用于 macOS)
generate_xcodeproj() {
    local project_name="$1"
    local project_path="$2"
    local xcode_dir="$project_path/$project_name.xcodeproj"

    mkdir -p "$xcode_dir"

    # 收集源文件
    local source_files=()
    local header_files=()

    # 扫描源文件
    while IFS= read -r -d '' file; do
        local relative_file=$(get_relative_path "$file" "$project_path")
        source_files+=("$relative_file")
    done < <(find "$project_path" -name "*.cpp" -print0)

    # 扫描头文件
    while IFS= read -r -d '' file; do
        local relative_file=$(get_relative_path "$file" "$project_path")
        header_files+=("$relative_file")
    done < <(find "$project_path" -name "*.h" -print0)

    # 判断项目类型
    local product_type="com.apple.product-type.library.static"
    if find "$project_path" -name "*Main.cpp" -o -name "main.cpp" | grep -q .; then
        product_type="com.apple.product-type.tool"
    fi

    # 生成 project.pbxproj
    cat >"$xcode_dir/project.pbxproj" <<EOF
// !$*UTF8*$!
{
	archiveVersion = 1;
	classes = {
	};
	objectVersion = 56;
	objects = {
		/* Begin PBXBuildFile section */
EOF

    # 添加源文件引用
    for file in "${source_files[@]}"; do
        local file_id=$(echo "$file" | md5sum | cut -c1-24 | tr '[:lower:]' '[:upper:]')
        echo "		$file_id /* $file in Sources */ = {isa = PBXBuildFile; fileRef = $(echo "$file" | md5sum | cut -c1-24 | tr '[:lower:]' '[:upper:]') /* $file */; };" >>"$xcode_dir/project.pbxproj"
    done

    cat >>"$xcode_dir/project.pbxproj" <<'EOF'
		/* End PBXBuildFile section */
		
		/* Begin PBXFileReference section */
EOF

    # 添加文件引用
    for file in "${source_files[@]}"; do
        local file_id=$(echo "$file" | md5sum | cut -c1-24 | tr '[:lower:]' '[:upper:]')
        echo "		$file_id /* $file */ = {isa = PBXFileReference; lastKnownFileType = sourcecode.cpp.cpp; name = $(basename "$file"); path = $file; sourceTree = \"<group>\"; };" >>"$xcode_dir/project.pbxproj"
    done

    for file in "${header_files[@]}"; do
        local file_id=$(echo "$file" | md5sum | cut -c1-24 | tr '[:lower:]' '[:upper:]')
        echo "		$file_id /* $file */ = {isa = PBXFileReference; lastKnownFileType = sourcecode.c.h; name = $(basename "$file"); path = $file; sourceTree = \"<group>\"; };" >>"$xcode_dir/project.pbxproj"
    done

    cat >>"$xcode_dir/project.pbxproj" <<'EOF'
		/* End PBXFileReference section */
		
		/* Begin PBXFrameworksBuildPhase section */
		/* End PBXFrameworksBuildPhase section */
		
		/* Begin PBXGroup section */
		/* Sources */ = {
			isa = PBXGroup;
			children = (
EOF

    # 添加源文件到组
    for file in "${source_files[@]}"; do
        local file_id=$(echo "$file" | md5sum | cut -c1-24 | tr '[:lower:]' '[:upper:]')
        echo "				$file_id /* $file */," >>"$xcode_dir/project.pbxproj"
    done

    for file in "${header_files[@]}"; do
        local file_id=$(echo "$file" | md5sum | cut -c1-24 | tr '[:lower:]' '[:upper:]')
        echo "				$file_id /* $file */," >>"$xcode_dir/project.pbxproj"
    done

    cat >>"$xcode_dir/project.pbxproj" <<'EOF'
			);
			path = Sources;
			sourceTree = "<group>";
		};
		/* End PBXGroup section */
		
		/* Begin PBXNativeTarget section */
		/* $project_name */ = {
			isa = PBXNativeTarget;
			buildConfigurationList = 1234567890ABCDEF /* Build configuration list for PBXNativeTarget "$project_name" */;
			buildPhases = (
				/* Sources */,
			);
			buildRules = (
			);
			dependencies = (
			);
			name = "$project_name";
			productName = "$project_name";
			productReference = 1234567890ABCDEF /* $project_name */;
			productType = "$product_type";
		};
		/* End PBXNativeTarget section */
		
		/* Begin XCBuildConfiguration section */
		/* Debug */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ALWAYS_SEARCH_USER_PATHS = NO;
				CLANG_ANALYZER_NONNULL = YES;
				CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
				CLANG_CXX_LANGUAGE_STANDARD = "gnu++17";
				CLANG_ENABLE_MODULES = YES;
				CLANG_ENABLE_OBJC_ARC = YES;
				CLANG_ENABLE_OBJC_WEAK = YES;
				CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING = YES;
				CLANG_WARN_BOOL_CONVERSION = YES;
				CLANG_WARN_COMMA = YES;
				CLANG_WARN_CONSTANT_CONVERSION = YES;
				CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS = YES;
				CLANG_WARN_DIRECT_OBJC_ISA_USAGE = YES_ERROR;
				CLANG_WARN_DOCUMENTATION_COMMENTS = YES;
				CLANG_WARN_EMPTY_BODY = YES;
				CLANG_WARN_ENUM_CONVERSION = YES;
				CLANG_WARN_INFINITE_RECURSION = YES;
				CLANG_WARN_INT_CONVERSION = YES;
				CLANG_WARN_NON_LITERAL_NULL_CONVERSION = YES;
				CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF = YES;
				CLANG_WARN_OBJC_LITERAL_CONVERSION = YES;
				CLANG_WARN_OBJC_ROOT_CLASS = YES_ERROR;
				CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER = YES;
				CLANG_WARN_RANGE_LOOP_ANALYSIS = YES;
				CLANG_WARN_STRICT_PROTOTYPES = YES;
				CLANG_WARN_SUSPICIOUS_MOVE = YES;
				CLANG_WARN_UNGUARDED_AVAILABILITY = YES_AGGRESSIVE;
				CLANG_WARN_UNREACHABLE_CODE = YES;
				CLANG_WARN__DUPLICATE_METHOD_MATCH = YES;
				COPY_PHASE_STRIP = NO;
				DEBUG_INFORMATION_FORMAT = dwarf;
				ENABLE_STRICT_OBJC_MSGSEND = YES;
				ENABLE_TESTABILITY = YES;
				GCC_C_LANGUAGE_STANDARD = gnu11;
				GCC_DYNAMIC_NO_PIC = NO;
				GCC_NO_COMMON_BLOCKS = YES;
				GCC_OPTIMIZATION_LEVEL = 0;
				GCC_PREPROCESSOR_DEFINITIONS = (
					"DEBUG=1",
					"$(inherited)",
				);
				GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
				GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
				GCC_WARN_UNDECLARED_SELECTOR = YES;
				GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
				GCC_WARN_UNUSED_FUNCTION = YES;
				GCC_WARN_UNUSED_VARIABLE = YES;
				MACOSX_DEPLOYMENT_TARGET = 10.15;
				MTL_ENABLE_DEBUG_INFO = INCLUDE_SOURCE;
				MTL_FAST_MATH = YES;
				ONLY_ACTIVE_ARCH = YES;
				SDKROOT = macosx;
			};
			name = Debug;
		};
		/* Release */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ALWAYS_SEARCH_USER_PATHS = NO;
				CLANG_ANALYZER_NONNULL = YES;
				CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
				CLANG_CXX_LANGUAGE_STANDARD = "gnu++17";
				CLANG_ENABLE_MODULES = YES;
				CLANG_ENABLE_OBJC_ARC = YES;
				CLANG_ENABLE_OBJC_WEAK = YES;
				CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING = YES;
				CLANG_WARN_BOOL_CONVERSION = YES;
				CLANG_WARN_COMMA = YES;
				CLANG_WARN_CONSTANT_CONVERSION = YES;
				CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS = YES;
				CLANG_WARN_DIRECT_OBJC_ISA_USAGE = YES_ERROR;
				CLANG_WARN_DOCUMENTATION_COMMENTS = YES;
				CLANG_WARN_EMPTY_BODY = YES;
				CLANG_WARN_ENUM_CONVERSION = YES;
				CLANG_WARN_INFINITE_RECURSION = YES;
				CLANG_WARN_INT_CONVERSION = YES;
				CLANG_WARN_NON_LITERAL_NULL_CONVERSION = YES;
				CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF = YES;
				CLANG_WARN_OBJC_LITERAL_CONVERSION = YES;
				CLANG_WARN_OBJC_ROOT_CLASS = YES_ERROR;
				CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER = YES;
				CLANG_WARN_RANGE_LOOP_ANALYSIS = YES;
				CLANG_WARN_STRICT_PROTOTYPES = YES;
				CLANG_WARN_SUSPICIOUS_MOVE = YES;
				CLANG_WARN_UNGUARDED_AVAILABILITY = YES_AGGRESSIVE;
				CLANG_WARN_UNREACHABLE_CODE = YES;
				CLANG_WARN__DUPLICATE_METHOD_MATCH = YES;
				COPY_PHASE_STRIP = NO;
				DEBUG_INFORMATION_FORMAT = "dwarf-with-dsym";
				ENABLE_NS_ASSERTIONS = NO;
				ENABLE_STRICT_OBJC_MSGSEND = YES;
				GCC_C_LANGUAGE_STANDARD = gnu11;
				GCC_NO_COMMON_BLOCKS = YES;
				GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
				GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
				GCC_WARN_UNDECLARED_SELECTOR = YES;
				GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
				GCC_WARN_UNUSED_FUNCTION = YES;
				GCC_WARN_UNUSED_VARIABLE = YES;
				MACOSX_DEPLOYMENT_TARGET = 10.15;
				MTL_ENABLE_DEBUG_INFO = NO;
				MTL_FAST_MATH = YES;
				SDKROOT = macosx;
			};
			name = Release;
		};
		/* End XCBuildConfiguration section */
		
		/* Begin XCConfigurationList section */
		/* Build configuration list for PBXProject "$project_name" */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				/* Debug */,
				/* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
		/* End XCConfigurationList section */
	};
	rootObject = 1234567890ABCDEF /* Project object */;
}
EOF

    echo "✅ 已生成 Xcode 项目文件: $xcode_dir"
}

# 生成 Visual Studio 项目文件 (.vcxproj) - Rider 兼容版本
generate_vcxproj() {
    local project_name="$1"
    local project_path="$2"
    local project_guid="$3"
    local vcxproj_path="$project_path/$project_name.vcxproj"

    # 收集源文件
    local source_files=()
    local header_files=()

    # 扫描源文件
    while IFS= read -r -d '' file; do
        source_files+=("$(get_relative_path "$file" "$project_path")")
    done < <(find "$project_path" -name "*.cpp" -print0)

    # 扫描头文件
    while IFS= read -r -d '' file; do
        header_files+=("$(get_relative_path "$file" "$project_path")")
    done < <(find "$project_path" -name "*.h" -print0)

    # 判断项目类型（Application/StaticLibrary）
    local config_type="StaticLibrary"
    if find "$project_path" -name "*Main.cpp" -o -name "main.cpp" | grep -q .; then
        config_type="Application"
    fi

    # 判断平台
    local is_windows=0
    case "$OSTYPE" in
    msys* | cygwin* | win32*) is_windows=1 ;;
    esac

    if [ $is_windows -eq 1 ]; then
        # Windows 下生成完整 vcxproj
        cat >"$vcxproj_path" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|Win32">
      <Configuration>Debug</Configuration>
      <Platform>Win32</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|Win32">
      <Configuration>Release</Configuration>
      <Platform>Win32</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <VCProjectVersion>16.0</VCProjectVersion>
    <Keyword>Win32Proj</Keyword>
    <ProjectGuid>$project_guid</ProjectGuid>
    <RootNamespace>$project_name</RootNamespace>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
  </PropertyGroup>
  <Import Project="\$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'\$(Configuration)|\$(Platform)'=='Debug|Win32'" Label="Configuration">
    <ConfigurationType>$config_type</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <PropertyGroup Condition="'\$(Configuration)|\$(Platform)'=='Release|Win32'" Label="Configuration">
    <ConfigurationType>$config_type</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
    <WholeProgramOptimization>true</WholeProgramOptimization>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <PropertyGroup Condition="'\$(Configuration)|\$(Platform)'=='Debug|x64'" Label="Configuration">
    <ConfigurationType>$config_type</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <PropertyGroup Condition="'\$(Configuration)|\$(Platform)'=='Release|x64'" Label="Configuration">
    <ConfigurationType>$config_type</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
    <WholeProgramOptimization>true</WholeProgramOptimization>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <Import Project="\$(VCTargetsPath)\\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Label="Shared">
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'\$(Configuration)|\$(Platform)'=='Debug|Win32'">
    <Import Project="\$(UserRootDir)\\Microsoft.Cpp.\$(Platform).user.props" Condition="exists('\$(UserRootDir)\\Microsoft.Cpp.\$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'\$(Configuration)|\$(Platform)'=='Release|Win32'">
    <Import Project="\$(UserRootDir)\\Microsoft.Cpp.\$(Platform).user.props" Condition="exists('\$(UserRootDir)\\Microsoft.Cpp.\$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'\$(Configuration)|\$(Platform)'=='Debug|x64'">
    <Import Project="\$(UserRootDir)\\Microsoft.Cpp.\$(Platform).user.props" Condition="exists('\$(UserRootDir)\\Microsoft.Cpp.\$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'\$(Configuration)|\$(Platform)'=='Release|x64'">
    <Import Project="\$(UserRootDir)\\Microsoft.Cpp.\$(Platform).user.props" Condition="exists('\$(UserRootDir)\\Microsoft.Cpp.\$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'\$(Configuration)|\$(Platform)'=='Debug|Win32'">
    <LinkIncremental>true</LinkIncremental>
  </PropertyGroup>
  <PropertyGroup Condition="'\$(Configuration)|\$(Platform)'=='Release|Win32'">
    <LinkIncremental>false</LinkIncremental>
  </PropertyGroup>
  <PropertyGroup Condition="'\$(Configuration)|\$(Platform)'=='Debug|x64'">
    <LinkIncremental>true</LinkIncremental>
  </PropertyGroup>
  <PropertyGroup Condition="'\$(Configuration)|\$(Platform)'=='Release|x64'">
    <LinkIncremental>false</LinkIncremental>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'\$(Configuration)|\$(Platform)'=='Debug|Win32'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>WIN32;_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp17</LanguageStandard>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'\$(Configuration)|\$(Platform)'=='Release|Win32'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>WIN32;NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp17</LanguageStandard>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <EnableCOMDATFolding>true</EnableCOMDATFolding>
      <OptimizeReferences>true</OptimizeReferences>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'\$(Configuration)|\$(Platform)'=='Debug|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp17</LanguageStandard>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'\$(Configuration)|\$(Platform)'=='Release|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp17</LanguageStandard>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <EnableCOMDATFolding>true</EnableCOMDATFolding>
      <OptimizeReferences>true</OptimizeReferences>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
EOF
    else
        # macOS/Linux 下生成 Rider 兼容的 vcxproj
        cat >"$vcxproj_path" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <ProjectGuid>$project_guid</ProjectGuid>
    <RootNamespace>$project_name</RootNamespace>
    <VCProjectVersion>16.0</VCProjectVersion>
  </PropertyGroup>
  <PropertyGroup Condition="'\$(Configuration)|\$(Platform)'=='Debug|x64'" Label="Configuration">
    <ConfigurationType>$config_type</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'\$(Configuration)|\$(Platform)'=='Release|x64'" Label="Configuration">
    <ConfigurationType>$config_type</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'\$(Configuration)|\$(Platform)'=='Debug|x64'">
    <LinkIncremental>true</LinkIncremental>
  </PropertyGroup>
  <PropertyGroup Condition="'\$(Configuration)|\$(Platform)'=='Release|x64'">
    <LinkIncremental>false</LinkIncremental>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'\$(Configuration)|\$(Platform)'=='Debug|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp17</LanguageStandard>
      <AdditionalIncludeDirectories>\$(ProjectDir);\$(ProjectDir)Sources;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <CompileAsManaged>false</CompileAsManaged>
      <TreatWChar_tAsBuiltInType>true</TreatWChar_tAsBuiltInType>
      <MinimalRebuild>false</MinimalRebuild>
      <ExceptionHandling>Sync</ExceptionHandling>
      <BasicRuntimeChecks>EnableFastChecks</BasicRuntimeChecks>
      <RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <ProgramDataBaseFileName>\$(IntDir)\$(TargetName).pdb</ProgramDataBaseFileName>
      <BrowseInformation>true</BrowseInformation>
      <SuppressStartupBanner>true</SuppressStartupBanner>
      <DebugInformationFormat>ProgramDatabase</DebugInformationFormat>
      <CompileAs>CompileAsCpp</CompileAs>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
      <EnableUAC>false</EnableUAC>
      <SuppressStartupBanner>true</SuppressStartupBanner>
      <AdditionalDependencies>%(AdditionalDependencies)</AdditionalDependencies>
      <OutputFile>\$(OutDir)\$(TargetName)\$(TargetExt)</OutputFile>
      <LinkIncremental>true</LinkIncremental>
      <ProgramDatabaseFile>\$(OutDir)\$(TargetName).pdb</ProgramDatabaseFile>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'\$(Configuration)|\$(Platform)'=='Release|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <LanguageStandard>stdcpp17</LanguageStandard>
      <AdditionalIncludeDirectories>\$(ProjectDir);\$(ProjectDir)Sources;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <CompileAsManaged>false</CompileAsManaged>
      <TreatWChar_tAsBuiltInType>true</TreatWChar_tAsBuiltInType>
      <MinimalRebuild>false</MinimalRebuild>
      <ExceptionHandling>Sync</ExceptionHandling>
      <BasicRuntimeChecks>Default</BasicRuntimeChecks>
      <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <ProgramDataBaseFileName>\$(IntDir)\$(TargetName).pdb</ProgramDataBaseFileName>
      <BrowseInformation>true</BrowseInformation>
      <SuppressStartupBanner>true</SuppressStartupBanner>
      <DebugInformationFormat>ProgramDatabase</DebugInformationFormat>
      <CompileAs>CompileAsCpp</CompileAs>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <EnableCOMDATFolding>true</EnableCOMDATFolding>
      <OptimizeReferences>true</OptimizeReferences>
      <GenerateDebugInformation>true</GenerateDebugInformation>
      <EnableUAC>false</EnableUAC>
      <SuppressStartupBanner>true</SuppressStartupBanner>
      <AdditionalDependencies>%(AdditionalDependencies)</AdditionalDependencies>
      <OutputFile>\$(OutDir)\$(TargetName)\$(TargetExt)</OutputFile>
      <LinkIncremental>false</LinkIncremental>
      <ProgramDatabaseFile>\$(OutDir)\$(TargetName).pdb</ProgramDatabaseFile>
    </Link>
  </ItemDefinitionGroup>
EOF
    fi

    # 添加源文件
    if [ ${#source_files[@]} -gt 0 ]; then
        echo "  <ItemGroup>" >>"$vcxproj_path"
        for file in "${source_files[@]}"; do
            echo "    <ClCompile Include=\"$file\" />" >>"$vcxproj_path"
        done
        echo "  </ItemGroup>" >>"$vcxproj_path"
    fi

    # 添加头文件
    if [ ${#header_files[@]} -gt 0 ]; then
        echo "  <ItemGroup>" >>"$vcxproj_path"
        for file in "${header_files[@]}"; do
            echo "    <ClInclude Include=\"$file\" />" >>"$vcxproj_path"
        done
        echo "  </ItemGroup>" >>"$vcxproj_path"
    fi

    # 结束标签
    echo "</Project>" >>"$vcxproj_path"

    echo "✅ 已生成 Visual Studio 项目文件: $vcxproj_path"
}

# 生成 Visual Studio 解决方案
generate_sln() {
    local projects=("$@")
    local sln_path="$PROJECT_ROOT/Nut.sln"

    # 创建解决方案文件
    cat >"$sln_path" <<'EOF'
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 17
VisualStudioVersion = 17.0.31903.59
MinimumVisualStudioVersion = 10.0.40219.1
EOF

    # 按分组组织项目
    local project_configs=()
    local nested_projects=()
    local groups=""

    # 收集所有分组
    for project in "${projects[@]}"; do
        IFS='|' read -r type name path group <<<"$project"
        groups=$(add_group "$group" "$groups")
    done

    # 添加项目
    for project in "${projects[@]}"; do
        IFS='|' read -r type name path group <<<"$project"
        local guid=$(generate_guid "$name")

        if [ "$type" = "csharp" ]; then
            local project_type_guid="{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}"
            local project_path="$path/$name.csproj"
        else
            local project_type_guid="{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}"
            local project_path="$path/$name.vcxproj"
            generate_vcxproj "$name" "$PROJECT_ROOT/$path" "$guid"
            # 在 macOS 下同时生成 Xcode 项目
            if [[ "$OSTYPE" != "msys"* && "$OSTYPE" != "cygwin"* && "$OSTYPE" != "win32"* ]]; then
                generate_xcodeproj "$name" "$PROJECT_ROOT/$path"
            fi
        fi

        echo "Project(\"$project_type_guid\") = \"$name\", \"$project_path\", \"$guid\"" >>"$sln_path"
        echo "EndProject" >>"$sln_path"

        project_configs+=("$guid.Debug|Any CPU.ActiveCfg = Debug|Any CPU")
        project_configs+=("$guid.Debug|Any CPU.Build.0 = Debug|Any CPU")
        project_configs+=("$guid.Release|Any CPU.ActiveCfg = Release|Any CPU")
        project_configs+=("$guid.Release|Any CPU.Build.0 = Release|Any CPU")

        # 为嵌套项目准备
        local group_guid=$(generate_guid "Folder_$group")
        nested_projects+=("$guid = $group_guid")
    done

    # 添加分组文件夹
    IFS='|' read -ra group_array <<<"$groups"
    for group in "${group_array[@]}"; do
        if [ -n "$group" ]; then
            local group_guid=$(generate_guid "Folder_$group")
            echo "Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"$group\", \"$group\", \"$group_guid\"" >>"$sln_path"
            echo "EndProject" >>"$sln_path"
        fi
    done

    # 添加全局配置
    cat >>"$sln_path" <<'EOF'
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|Any CPU = Debug|Any CPU
		Release|Any CPU = Release|Any CPU
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
EOF

    for config in "${project_configs[@]}"; do
        echo "		$config" >>"$sln_path"
    done

    # 添加嵌套项目配置
    cat >>"$sln_path" <<'EOF'
	EndGlobalSection
	GlobalSection(NestedProjects) = preSolution
EOF

    # 添加嵌套项目
    for nested in "${nested_projects[@]}"; do
        echo "		$nested" >>"$sln_path"
    done

    cat >>"$sln_path" <<'EOF'
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
EndGlobal
EOF

    echo "✅ 已生成 Visual Studio 解决方案: $sln_path"
}

# 主函数
main() {
    local action="${1:-generate}"

    case "$action" in
    "setup")
        echo "🔧 正在创建基础项目文件..."
        generate_sln
        echo "✅ 基础项目文件创建完成！"
        ;;
    "generate")
        echo "🔍 正在发现项目..."
        local projects=($(discover_projects))

        if [ ${#projects[@]} -eq 0 ]; then
            echo "⚠️  未发现任何项目"
            return
        fi

        echo "📁 发现 ${#projects[@]} 个项目:"

        # 按分组显示项目
        local groups=""
        for project in "${projects[@]}"; do
            IFS='|' read -r type name path group <<<"$project"
            groups=$(add_group "$group" "$groups")
        done

        IFS='|' read -ra group_array <<<"$groups"
        for group in "${group_array[@]}"; do
            if [ -n "$group" ]; then
                echo "  📂 $group:"
                for project in "${projects[@]}"; do
                    IFS='|' read -r type name path project_group <<<"$project"
                    if [ "$project_group" = "$group" ]; then
                        echo "    - $name ($type)"
                    fi
                done
            fi
        done

        echo ""
        echo "🔨 正在生成项目文件..."
        generate_sln "${projects[@]}"
        echo ""
        echo "✅ 项目文件生成完成！"
        ;;
    *)
        echo "用法: $0 [setup|generate]"
        echo "  setup   - 创建基础项目文件"
        echo "  generate - 发现项目并更新项目文件"
        exit 1
        ;;
    esac
}

main "$@"

