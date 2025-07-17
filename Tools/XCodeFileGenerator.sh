#!/bin/bash

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

# 查找所有*.Build.cs和*.Build.py文件
find_build_files() {
    local source_dir="$PROJECT_ROOT/Source"
    local build_files=()
    
    echo "🔍 正在搜索Source目录下的Build文件..."
    
    while IFS= read -r -d '' dir; do
        if [[ -d "$dir/Meta" ]]; then
            while IFS= read -r -d '' build_file; do
                build_files+=("$build_file")
                echo "📁 找到Build文件: $build_file"
            done < <(find "$dir/Meta" -maxdepth 1 \( -name "*.Build.cs" -o -name "*.Build.py" \) -print0 2>/dev/null)
        fi
    done < <(find "$source_dir" -type d -print0 2>/dev/null)
    
    echo "✅ 共找到 ${#build_files[@]} 个Build文件"
    
    for file in "${build_files[@]}"; do
        echo "$file"
    done
}

# 解析Build文件获取项目信息
parse_build_file() {
    local build_file="$1"
    local project_dir=$(dirname "$(dirname "$build_file")")
    local project_type=""
    
    if [[ "$project_dir" == */"Programs/"* ]]; then
        project_type="Programs"
    elif [[ "$project_dir" == */"Runtime/"* ]]; then
        project_type="Runtime"
    fi
    
    local project_name=$(basename "$project_dir")
    local sources_dir="$project_dir/Sources"
    local meta_dir="$project_dir/Meta"
    local config_dir="$project_dir/Configs"
    
    local header_files=()
    local source_files=()
    local meta_files=()
    local config_files=()

    # 收集源文件和头文件
    if [[ -d "$sources_dir" ]]; then
        while IFS= read -r -d '' file; do
            local relative_path=$(python3 -c "import os; print(os.path.relpath('$file', '$PROJECT_ROOT'))")
            if [[ "$file" == *.h || "$file" == *.hpp ]]; then
                header_files+=("$relative_path")
            elif [[ "$file" == *.cpp || "$file" == *.c ]]; then
                source_files+=("$relative_path")
            fi
        done < <(find "$sources_dir" -type f \( -name "*.cpp" -o -name "*.c" -o -name "*.h" -o -name "*.hpp" \) -print0 2>/dev/null)
    fi

    # 收集Meta文件
    if [[ -d "$meta_dir" ]]; then
        while IFS= read -r -d '' file; do
            local relative_path=$(python3 -c "import os; print(os.path.relpath('$file', '$PROJECT_ROOT'))")
            meta_files+=("$relative_path")
        done < <(find "$meta_dir" -type f -print0 2>/dev/null)
    fi

    # 收集Config文件
    if [[ -d "$config_dir" ]]; then
        while IFS= read -r -d '' file; do
            local relative_path=$(python3 -c "import os; print(os.path.relpath('$file', '$PROJECT_ROOT'))")
            config_files+=("$relative_path")
        done < <(find "$config_dir" -type f -print0 2>/dev/null)
    fi

    echo "PROJECT_NAME=$project_name"
    echo "PROJECT_DIR=$project_dir"
    echo "PROJECT_TYPE=$project_type"
    
    for f in "${header_files[@]}"; do
        echo "HEADER:$f"
    done
    for f in "${source_files[@]}"; do
        echo "SOURCE:$f"
    done
    for f in "${meta_files[@]}"; do
        echo "META:$f"
    done
    for f in "${config_files[@]}"; do
        echo "CONFIG:$f"
    done
}

# 生成分组Xcode项目
generate_xcode_project_grouped() {
    local project_name="$1"
    local project_path="$2"
    local project_type="$3"
    shift 3
    
    # 解析分组参数
    local all_args=("$@")
    local header_files=()
    local source_files=()
    local meta_files=()
    local config_files=()
    
    local current_group=""
    for arg in "${all_args[@]}"; do
        if [[ "$arg" == "HEADER:"* ]]; then
            current_group="header"
            header_files+=("${arg#HEADER:}")
        elif [[ "$arg" == "SOURCE:"* ]]; then
            current_group="source"
            source_files+=("${arg#SOURCE:}")
        elif [[ "$arg" == "META:"* ]]; then
            current_group="meta"
            meta_files+=("${arg#META:}")
        elif [[ "$arg" == "CONFIG:"* ]]; then
            current_group="config"
            config_files+=("${arg#CONFIG:}")
        else
            case "$current_group" in
                "header") header_files+=("$arg") ;;
                "source") source_files+=("$arg") ;;
                "meta") meta_files+=("$arg") ;;
                "config") config_files+=("$arg") ;;
            esac
        fi
    done

    echo "🔨 正在生成分组Xcode项目: $project_name"
    
    # 创建项目目录
    local projects_dir="$PROJECT_ROOT/Projects"
    mkdir -p "$projects_dir/$project_type/$project_name"
    local project_dir="$projects_dir/$project_type/$project_name/$project_name.xcodeproj"
    mkdir -p "$project_dir"

    # 生成UUID
    local project_uuid=$(generate_random_uuid)
    local main_group_uuid=$(generate_random_uuid)
    local headers_group_uuid=$(generate_random_uuid)
    local sources_group_uuid=$(generate_random_uuid)
    local meta_group_uuid=$(generate_random_uuid)
    local configs_group_uuid=$(generate_random_uuid)
    local target_uuid=$(generate_random_uuid)
    local product_ref_uuid=$(generate_random_uuid)
    local build_phase_uuid=$(generate_random_uuid)
    
    # 配置UUID
    local project_debug_uuid=$(generate_random_uuid)
    local project_release_uuid=$(generate_random_uuid)
    local target_debug_uuid=$(generate_random_uuid)
    local target_release_uuid=$(generate_random_uuid)
    local config_list_project_uuid=$(generate_random_uuid)
    local config_list_target_uuid=$(generate_random_uuid)

    # 处理所有文件
    local all_files=("${header_files[@]}" "${source_files[@]}" "${meta_files[@]}" "${config_files[@]}")
    local file_refs=()
    local build_files=()
    local file_indices=()
    
    # 只为源文件生成build file
    for i in "${!source_files[@]}"; do
        build_files+=("$(generate_random_uuid)")
        file_indices+=("$((${#header_files[@]} + $i))")
    done
    
    # 为所有文件生成引用
    for i in "${!all_files[@]}"; do
        file_refs+=("$(generate_random_uuid)")
    done

    # 生成project.pbxproj文件
    local pbxproj="$project_dir/project.pbxproj"
    
    # 生成文件头
    cat > "$pbxproj" << EOF
// !\$*UTF8*\$!
{
	archiveVersion = 1;
	classes = {
	};
	objectVersion = 46;
	objects = {
EOF

    # 添加Project对象
    cat >> "$pbxproj" << EOF
		$project_uuid /* Project object */ = {
			isa = PBXProject;
			attributes = {
				LastUpgradeCheck = 9999;
			};
			buildConfigurationList = $config_list_project_uuid;
			compatibilityVersion = "Xcode 3.2";
			mainGroup = $main_group_uuid;
			targets = (
				$target_uuid,
			);
		};
EOF

    # 添加Main Group
    cat >> "$pbxproj" << EOF
		$main_group_uuid /* Main Group */ = {
			isa = PBXGroup;
			children = (
				$headers_group_uuid /* Headers */,
				$sources_group_uuid /* Sources */,
				$meta_group_uuid /* Meta */,
				$configs_group_uuid /* Configs */,
			);
			sourceTree = "<group>";
		};
EOF

    # 添加Headers Group
    cat >> "$pbxproj" << EOF
		$headers_group_uuid /* Headers */ = {
			isa = PBXGroup;
			children = (
EOF
    for i in "${!header_files[@]}"; do
        printf "\t\t\t\t%s /* %s */,\n" "${file_refs[$i]}" "${header_files[$i]}" >> "$pbxproj"
    done
    cat >> "$pbxproj" << EOF
			);
			name = "Headers";
			sourceTree = "<group>";
		};
EOF

    # 添加Sources Group
    cat >> "$pbxproj" << EOF
		$sources_group_uuid /* Sources */ = {
			isa = PBXGroup;
			children = (
EOF
    for i in "${!source_files[@]}"; do
        local idx=$((${#header_files[@]} + $i))
        printf "\t\t\t\t%s /* %s */,\n" "${file_refs[$idx]}" "${source_files[$i]}" >> "$pbxproj"
    done
    cat >> "$pbxproj" << EOF
			);
			name = "Sources";
			sourceTree = "<group>";
		};
EOF

    # 添加Meta Group
    cat >> "$pbxproj" << EOF
		$meta_group_uuid /* Meta */ = {
			isa = PBXGroup;
			children = (
EOF
    for i in "${!meta_files[@]}"; do
        local idx=$((${#header_files[@]} + ${#source_files[@]} + $i))
        printf "\t\t\t\t%s /* %s */,\n" "${file_refs[$idx]}" "${meta_files[$i]}" >> "$pbxproj"
    done
    cat >> "$pbxproj" << EOF
			);
			name = "Meta";
			sourceTree = "<group>";
		};
EOF

    # 添加Configs Group
    cat >> "$pbxproj" << EOF
		$configs_group_uuid /* Configs */ = {
			isa = PBXGroup;
			children = (
EOF
    for i in "${!config_files[@]}"; do
        local idx=$((${#header_files[@]} + ${#source_files[@]} + ${#meta_files[@]} + $i))
        printf "\t\t\t\t%s /* %s */,\n" "${file_refs[$idx]}" "${config_files[$i]}" >> "$pbxproj"
    done
    cat >> "$pbxproj" << EOF
			);
			name = "Configs";
			sourceTree = "<group>";
		};
EOF

    # 添加Target
    cat >> "$pbxproj" << EOF
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

    # 添加文件引用
    for i in "${!all_files[@]}"; do
        local file_type="text"
        case "${all_files[$i]}" in
            *.h) file_type="sourcecode.c.h" ;;
            *.hpp) file_type="sourcecode.cpp.h" ;;
            *.cpp) file_type="sourcecode.cpp.cpp" ;;
            *.c) file_type="sourcecode.c.c" ;;
            *.json) file_type="text.json" ;;
            *.cs|*.py) file_type="text" ;;
        esac
        
        local file_abs="$PROJECT_ROOT/${all_files[$i]}"
        local rel_path=$(python3 -c "import os; print(os.path.relpath('$file_abs', '$projects_dir/$project_name'))")
        
        cat >> "$pbxproj" << EOF
		${file_refs[$i]} /* $(basename "${all_files[$i]}") */ = {
			isa = PBXFileReference;
			lastKnownFileType = $file_type;
			path = "$rel_path";
			name = "$(basename "${all_files[$i]}")";
			sourceTree = "SOURCE_ROOT";
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

    # 添加BuildFile
    for i in "${!file_indices[@]}"; do
        local idx=${file_indices[$i]}
        cat >> "$pbxproj" << EOF
		${build_files[$i]} /* ${all_files[$idx]} in Sources */ = {
			isa = PBXBuildFile;
			fileRef = ${file_refs[$idx]};
		};
EOF
    done

    # 添加BuildPhase
    cat >> "$pbxproj" << EOF
		$build_phase_uuid /* Sources */ = {
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
EOF
    for i in "${!file_indices[@]}"; do
        local idx=${file_indices[$i]}
        printf "\t\t\t\t%s /* %s in Sources */,\n" "${build_files[$i]}" "${all_files[$idx]}" >> "$pbxproj"
    done
    cat >> "$pbxproj" << EOF
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
EOF

    # 添加构建配置
    cat >> "$pbxproj" << EOF
		$project_debug_uuid /* Debug */ = {
			isa = XCBuildConfiguration;
			name = Debug;
			buildSettings = {
				PRODUCT_NAME = "\$(TARGET_NAME)";
			};
		};
		$project_release_uuid /* Release */ = {
			isa = XCBuildConfiguration;
			name = Release;
			buildSettings = {
				PRODUCT_NAME = "\$(TARGET_NAME)";
			};
		};
		$target_debug_uuid /* Debug */ = {
			isa = XCBuildConfiguration;
			name = Debug;
			buildSettings = {
				PRODUCT_NAME = "\$(TARGET_NAME)";
			};
		};
		$target_release_uuid /* Release */ = {
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

# 创建Workspace
create_workspace() {
    local workspace_dir="$PROJECT_ROOT/Nut.xcworkspace"
    local workspace_data="$workspace_dir/contents.xcworkspacedata"
    mkdir -p "$workspace_dir"
    
    local programs=()
    local runtimes=()
    
    if [[ -d "$PROJECT_ROOT/Projects" ]]; then
        while IFS= read -r -d '' proj; do
            local proj_name=$(basename "$proj" .xcodeproj)
            local proj_path=$(python3 -c "import os; print(os.path.relpath('$proj', '$PROJECT_ROOT'))")
            
            if [[ "$proj" == */Programs/* ]]; then
                programs+=("$proj_path")
            elif [[ "$proj" == */Runtime/* ]]; then
                runtimes+=("$proj_path")
            fi
        done < <(find "$PROJECT_ROOT/Projects" -name "*.xcodeproj" -print0 2>/dev/null)
    fi
    
    cat > "$workspace_data" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<Workspace version = "1.0">
EOF

    if [[ ${#programs[@]} -gt 0 ]]; then
        echo "  <Group location = \"container:\" name = \"Programs\">" >> "$workspace_data"
        for proj in "${programs[@]}"; do
            echo "    <FileRef location = \"group:$proj\"/>" >> "$workspace_data"
        done
        echo "  </Group>" >> "$workspace_data"
    fi
    
    if [[ ${#runtimes[@]} -gt 0 ]]; then
        echo "  <Group location = \"container:\" name = \"Runtime\">" >> "$workspace_data"
        for proj in "${runtimes[@]}"; do
            echo "    <FileRef location = \"group:$proj\"/>" >> "$workspace_data"
        done
        echo "  </Group>" >> "$workspace_data"
    fi
    
    echo "</Workspace>" >> "$workspace_data"
    echo "✅ Nut.xcworkspace 已生成于项目根目录"
}

# 主执行逻辑
main() {
    echo "🚀 开始生成Xcode项目..."
    
    # 查找所有Build.cs文件
    local build_files=()
    while IFS= read -r line; do
        build_files+=("$line")
    done < <(find_build_files)
    
    if [[ ${#build_files[@]} -eq 0 ]]; then
        echo "❌ 未找到任何Build文件"
        exit 1
    fi
    
    echo "📋 找到的Build文件列表:"
    printf '  - %s\n' "${build_files[@]}"
    
    # 为每个Build文件生成对应的Xcode项目
    for build_file in "${build_files[@]}"; do
        echo ""
        echo "🔧 处理Build文件: $build_file"
        
        # 解析Build文件获取项目信息
        local project_info=$(parse_build_file "$build_file")
        
        # 提取项目信息
        local project_name=""
        local project_dir=""
        local project_type=""
        local header_files=()
        local source_files=()
        local meta_files=()
        local config_files=()
        
        while IFS= read -r line; do
            case "$line" in
                PROJECT_NAME=*) project_name="${line#PROJECT_NAME=}" ;;
                PROJECT_DIR=*) project_dir="${line#PROJECT_DIR=}" ;;
                PROJECT_TYPE=*) project_type="${line#PROJECT_TYPE=}" ;;
                HEADER:*) header_files+=("${line#HEADER:}") ;;
                SOURCE:*) source_files+=("${line#SOURCE:}") ;;
                META:*) meta_files+=("${line#META:}") ;;
                CONFIG:*) config_files+=("${line#CONFIG:}") ;;
            esac
        done <<< "$project_info"
        
        # 生成Xcode项目
        if [[ -n "$project_name" && "$project_name" != "." ]]; then
            generate_xcode_project_grouped "$project_name" "$project_dir" "$project_type" \
                "${header_files[@]/#/HEADER:}" \
                "${source_files[@]/#/SOURCE:}" \
                "${meta_files[@]/#/META:}" \
                "${config_files[@]/#/CONFIG:}"
        fi
    done
    
    # 创建Workspace
    create_workspace
    
    echo ""
    echo "🎉 所有Xcode项目生成完成！"
}

# 执行主函数
main