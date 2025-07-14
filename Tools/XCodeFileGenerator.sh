#!/bin/bash

# 简化版 Xcode 工程生成脚本
# 用法：在项目根目录准备 project.conf 文件

set -e

PROJECT_ROOT=$(cd "$(dirname "$0")/.."; pwd)

echo "当前项目路径: $PROJECT_ROOT"

PROJECT_NAME=$(basename "$PROJECT_ROOT")

echo "当前项目名称: $PROJECT_NAME"

# UUID 生成函数
generate_random_uuid() {
    if command -v uuidgen >/dev/null 2>&1; then
        uuidgen | tr -d '-' | cut -c1-24
    elif command -v openssl >/dev/null 2>&1; then
        openssl rand -hex 12 | tr 'a-f' 'A-F'
    else
        echo "无法生成UUID" >&2
        exit 1
    fi
}

# 生成 UUID
PROJECT_UUID=$(generate_random_uuid)
TARGET_UUID=$(generate_random_uuid)
GROUP_UUID=$(generate_random_uuid)
PROJECT_DEBUG_UUID=$(generate_random_uuid)
PROJECT_RELEASE_UUID=$(generate_random_uuid)
TARGET_DEBUG_UUID=$(generate_random_uuid)
TARGET_RELEASE_UUID=$(generate_random_uuid)
CONFIG_LIST_PROJECT_UUID=$(generate_random_uuid)
CONFIG_LIST_TARGET_UUID=$(generate_random_uuid)
BUILD_PHASE_UUID=$(generate_random_uuid)
PRODUCT_REF_UUID=$(generate_random_uuid)

# 处理源文件
IFS=',' read -ra FILES <<< "$SRC_FILES"
FILE_REFS=()
BUILD_FILES=()
for f in "${FILES[@]}"; do
    FILE_REFS+=("$(generate_random_uuid)")
    BUILD_FILES+=("$(generate_random_uuid)")
done

# 1. 在脚本开头定义中间目录
INTERMEDIATE_DIR="$PROJECT_ROOT/Intermediate"
mkdir -p "$INTERMEDIATE_DIR"

# 遍历Source路径下的所有文件夹，搜索Meta目录下面的*.Build.cs文件，用来在workspace下面创建project

# 查找所有*.Build.cs和*.Build.py文件
find_build_files() {
    local source_dir="$PROJECT_ROOT/Source"
    local build_files=()
    
    echo "🔍 正在搜索Source目录下的*.Build.cs和*.Build.py文件..."
    
    # 遍历Source目录下的所有子目录
    while IFS= read -r -d '' dir; do
        # 检查是否存在Meta目录
        if [[ -d "$dir/Meta" ]]; then
            # 在Meta目录中查找*.Build.cs和*.Build.py文件
            while IFS= read -r -d '' build_file; do
                if [[ -f "$build_file" ]]; then
                    build_files+=("$build_file")
                    echo "📁 找到Build文件: $build_file"
                fi
            done < <(find "$dir/Meta" -maxdepth 1 \( -name "*.Build.cs" -o -name "*.Build.py" \) -print0 2>/dev/null)
        fi
    done < <(find "$source_dir" -type d -print0 2>/dev/null)
    
    echo "✅ 共找到 ${#build_files[@]} 个Build文件"
    
    # 返回找到的文件数组
    for file in "${build_files[@]}"; do
        echo "$file"
    done
}

# 解析Build.cs文件获取项目信息
parse_build_file() {
    local build_file="$1"
    local project_dir=$(dirname "$(dirname "$build_file")")  # 回到项目根目录
    
    echo "📋 解析Build文件: $build_file"
    echo "📁 项目目录: $project_dir"
    
    # 这里可以根据需要解析Build.cs文件的内容
    # 目前先返回项目目录名作为项目名
    local project_name=$(basename "$project_dir")
    echo "🏷️  项目名称: $project_name"
    
    # 查找源文件
    local sources_dir="$project_dir/Sources"
    local source_files=()
    
    if [[ -d "$sources_dir" ]]; then
        while IFS= read -r -d '' file; do
            if [[ -f "$file" ]]; then
                # 获取相对于xcodeproj目录的路径
                local relative_path="${file#$project_dir/}"
                source_files+=("$relative_path")
            fi
        done < <(find "$sources_dir" -type f \( -name "*.cpp" -o -name "*.c" -o -name "*.h" -o -name "*.hpp" \) -print0 2>/dev/null)
    fi
    
    echo "📄 找到 ${#source_files[@]} 个源文件"
    
    # 返回项目信息
    echo "PROJECT_NAME=$project_name"
    echo "PROJECT_DIR=$project_dir"
    printf '%s\n' "${source_files[@]}"
}

# 生成Xcode项目
generate_xcode_project() {
    local project_name="$1"
    local project_path="$2"
    local source_files=("${@:3}")
    
    echo "🔨 正在生成Xcode项目: $project_name"
    
    # 创建中间目录
    local intermediate_proj_dir="$INTERMEDIATE_DIR/$project_name"
    mkdir -p "$intermediate_proj_dir"
    local project_dir="$intermediate_proj_dir/$project_name.xcodeproj"
    mkdir -p "$project_dir"
    
    # 生成UUID
    local project_uuid=$(generate_random_uuid)
    local target_uuid=$(generate_random_uuid)
    local group_uuid=$(generate_random_uuid)
    local project_debug_uuid=$(generate_random_uuid)
    local project_release_uuid=$(generate_random_uuid)
    local target_debug_uuid=$(generate_random_uuid)
    local target_release_uuid=$(generate_random_uuid)
    local config_list_project_uuid=$(generate_random_uuid)
    local config_list_target_uuid=$(generate_random_uuid)
    local build_phase_uuid=$(generate_random_uuid)
    local product_ref_uuid=$(generate_random_uuid)
    
    # 处理源文件UUID
    local file_refs=()
    local build_files=()
    for f in "${source_files[@]}"; do
        file_refs+=("$(generate_random_uuid)")
        build_files+=("$(generate_random_uuid)")
    done
    
    # 生成project.pbxproj文件
    local pbxproj="$project_dir/project.pbxproj"
    
    # 生成project.pbxproj内容
    cat > "$pbxproj" << EOF
// !$*UTF8*$!
{
	archiveVersion = 1;
	classes = {
	};
	objectVersion = 46;
	objects = {
		$project_uuid /* Project object */ = {
			isa = PBXProject;
			attributes = {
				LastUpgradeCheck = 9999;
			};
			buildConfigurationList = $config_list_project_uuid;
			compatibilityVersion = "Xcode 3.2";
			mainGroup = $group_uuid;
			targets = (
				$target_uuid,
			);
		};
		$group_uuid /* Main Group */ = {
			isa = PBXGroup;
			children = (
EOF

    # 添加文件引用
    for i in "${!source_files[@]}"; do
        echo "\t\t\t\t${file_refs[$i]} /* ${source_files[$i]} */," >> "$pbxproj"
    done

    cat >> "$pbxproj" << EOF
			);
			sourceTree = "<group>";
		};
		$target_uuid /* $project_name */ = {
			isa = PBXNativeTarget;
			name = "$project_name";
			productType = "com.apple.product-type.tool";
			buildConfigurationList = $config_list_target_uuid;
			buildPhases = (
				$build_phase_uuid,
			);
			buildRules = (
			);
			dependencies = (
			);
			productReference = $product_ref_uuid;
		};
EOF

    # 添加文件引用对象
    for i in "${!source_files[@]}"; do
    cat >> "$pbxproj" << EOF
		${file_refs[$i]} /* ${source_files[$i]} */ = {
			isa = PBXFileReference;
			lastKnownFileType = sourcecode.c.c;
			path = "${source_files[$i]}";
			sourceTree = "<group>";
		};
EOF
    done

    # 添加产品引用
    cat >> "$pbxproj" << EOF
		$product_ref_uuid /* $project_name */ = {
			isa = PBXFileReference;
			explicitFileType = "compiled.mach-o.executable";
			path = "$project_name";
			sourceTree = "BUILT_PRODUCTS_DIR";
		};
EOF

    # 添加构建文件
    for i in "${!source_files[@]}"; do
    cat >> "$pbxproj" << EOF
		${build_files[$i]} /* ${source_files[$i]} in Sources */ = {
			isa = PBXBuildFile;
			fileRef = ${file_refs[$i]};
		};
EOF
    done

    # 添加构建阶段
    cat >> "$pbxproj" << EOF
		$build_phase_uuid /* Sources */ = {
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
EOF

    for i in "${!source_files[@]}"; do
        echo "\t\t\t\t${build_files[$i]} /* ${source_files[$i]} in Sources */," >> "$pbxproj"
    done

    cat >> "$pbxproj" << EOF
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
EOF

    # 添加构建配置
    cat >> "$pbxproj" << EOF
		$project_debug_uuid /* Project Debug */ = {
			isa = XCBuildConfiguration;
			name = Debug;
			buildSettings = {
				PRODUCT_NAME = "\$(TARGET_NAME)";
			};
		};
		$project_release_uuid /* Project Release */ = {
			isa = XCBuildConfiguration;
			name = Release;
			buildSettings = {
				PRODUCT_NAME = "\$(TARGET_NAME)";
			};
		};
		$target_debug_uuid /* Target Debug */ = {
			isa = XCBuildConfiguration;
			name = Debug;
			buildSettings = {
				PRODUCT_NAME = "\$(TARGET_NAME)";
			};
		};
		$target_release_uuid /* Target Release */ = {
			isa = XCBuildConfiguration;
			name = Release;
			buildSettings = {
				PRODUCT_NAME = "\$(TARGET_NAME)";
			};
		};
EOF

    # 添加配置列表
    cat >> "$pbxproj" << EOF
		$config_list_project_uuid /* Build configuration list for PBXProject */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				$project_debug_uuid,
				$project_release_uuid,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
		$config_list_target_uuid /* Build configuration list for PBXNativeTarget */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				$target_debug_uuid,
				$target_release_uuid,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
	};
	rootObject = $project_uuid;
}
EOF

    echo "✅ Xcode项目已生成: $project_dir"
}

# 主执行逻辑
main() {
    echo "🚀 开始生成Xcode项目..."
    
    # 查找所有Build.cs文件
    local build_files=()
    while IFS= read -r line; do
        if [[ -n "$line" && "$line" != *"🔍"* && "$line" != *"📁"* && "$line" != *"✅"* ]]; then
            build_files+=("$line")
        fi
    done < <(find_build_files)
    
    if [[ ${#build_files[@]} -eq 0 ]]; then
        echo "❌ 未找到任何*.Build.cs或*.Build.py文件"
        exit 1
    fi
    
    echo "📋 找到的Build文件列表:"
    for file in "${build_files[@]}"; do
        echo "  - $file"
    done
    
    # 为每个Build.cs文件生成对应的Xcode项目
    for build_file in "${build_files[@]}"; do
        echo ""
        echo "🔧 处理Build文件: $build_file"
        
        # 解析Build文件获取项目信息
        local project_info=$(parse_build_file "$build_file")
        
        # 提取项目名称、项目路径和源文件
        local project_name=""
        local project_dir=""
        local source_files=()
        
        while IFS= read -r line; do
            if [[ $line =~ ^PROJECT_NAME= ]]; then
                project_name="${line#PROJECT_NAME=}"
            elif [[ $line =~ ^PROJECT_DIR= ]]; then
                project_dir="${line#PROJECT_DIR=}"
            elif [[ -n "$line" && "$line" != *"📋"* && "$line" != *"📁"* && "$line" != *"🏷️"* && "$line" != *"📄"* ]]; then
                # 其他行都是源文件
                source_files+=("$line")
            fi
        done <<< "$project_info"
        
        # 生成Xcode项目
        if [[ -n "$project_name" && "$project_name" != "." && -n "$project_dir" ]]; then
            generate_xcode_project "$project_name" "$project_dir" "${source_files[@]}"
        fi
    done
    
    echo ""
    echo "🎉 所有Xcode项目生成完成！"
}

# 执行主函数
main 

# 生成 Nut.xcworkspace 分组
# 修正 workspace 路径为相对于 Nut.xcworkspace 的路径
create_workspace() {
    local workspace_dir="$PROJECT_ROOT/Nut.xcworkspace"
    local workspace_data="$workspace_dir/contents.xcworkspacedata"
    mkdir -p "$workspace_dir"

    # 收集 Intermediate 下的所有 xcodeproj
    local programs=()
    local runtimes=()

    if [[ -d "$INTERMEDIATE_DIR" ]]; then
        while IFS= read -r -d '' proj; do
            rel_path="${proj#$PROJECT_ROOT/}"
            rel_path="${rel_path#./}"
            rel_path="${rel_path%/}"
            rel_path="${rel_path}" # Intermediate/项目名/项目名.xcodeproj
            programs+=("$rel_path")
        done < <(find "$INTERMEDIATE_DIR" -name "*.xcodeproj" -type d -print0)
    fi

    # 生成 XML
    cat > "$workspace_data" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<Workspace version = "1.0">
EOF
    if [[ ${#programs[@]} -gt 0 ]]; then
        echo "  <Group location = \"container:Programs\" name = \"Programs\">" >> "$workspace_data"
        for proj in "${programs[@]}"; do
            echo "    <FileRef location = \"group:${proj}\"/>" >> "$workspace_data"
        done
        echo "  </Group>" >> "$workspace_data"
    fi
    if [[ ${#runtimes[@]} -gt 0 ]]; then
        echo "  <Group location = \"container:Runtime\" name = \"Runtime\">" >> "$workspace_data"
        for proj in "${runtimes[@]}"; do
            echo "    <FileRef location = \"group:${proj}\"/>" >> "$workspace_data"
        done
        echo "  </Group>" >> "$workspace_data"
    fi
    echo "</Workspace>" >> "$workspace_data"
    echo "✅ Nut.xcworkspace 已生成于项目根目录，分组包含所有 xcodeproj"
}

# 在主流程最后调用
create_workspace 