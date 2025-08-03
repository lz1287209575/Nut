#!/bin/bash

# GenerateHeaders.sh - 运行NutHeaderTools生成.generate.h文件
# 集成到Nut引擎构建流程中

set -e

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# 定义路径
HEADER_TOOL_PROJECT="$PROJECT_ROOT/Source/Programs/NutHeaderTools/NutHeaderTools.csproj"
SOURCE_DIRS=(
    "$PROJECT_ROOT/Source/Runtime/LibNut/Sources"
    "$PROJECT_ROOT/Source/Runtime/MicroServices"
    "$PROJECT_ROOT/Source/Runtime/ServiceAllocate/Sources"
)

echo "🔧 Nut引擎头文件生成工具"
echo "========================================="

# 检查.NET是否安装
if ! command -v dotnet &>/dev/null; then
    echo "❌ 错误: 未找到.NET SDK，请安装.NET 8.0或更高版本"
    exit 1
fi

# 构建NutHeaderTools
echo "🏗️  构建NutHeaderTools..."
dotnet build "$HEADER_TOOL_PROJECT" --configuration Release --verbosity quiet

if [ $? -ne 0 ]; then
    echo "❌ NutHeaderTools构建失败"
    exit 1
fi

echo "✅ NutHeaderTools构建成功"

# 运行代码生成
echo ""
echo "🔍 开始生成头文件..."

TOTAL_GENERATED=0

for SOURCE_DIR in "${SOURCE_DIRS[@]}"; do
    if [ -d "$SOURCE_DIR" ]; then
        echo "📁 处理目录: $(basename "$SOURCE_DIR")"

        # 运行NutHeaderTools
        RESULT=$(dotnet run --project "$HEADER_TOOL_PROJECT" --configuration Release -- "$SOURCE_DIR" 2>&1)

        if [ $? -eq 0 ]; then
            # 从输出中提取生成的文件数量
            GENERATED_COUNT=$(echo "$RESULT" | grep -o "成功生成 [0-9]* 个" | grep -o "[0-9]*" || echo "0")
            TOTAL_GENERATED=$((TOTAL_GENERATED + GENERATED_COUNT))

            if [ "$GENERATED_COUNT" -gt 0 ]; then
                echo "   ✅ 生成了 $GENERATED_COUNT 个 .generate.h 文件"
            else
                echo "   ℹ️  该目录无需生成文件"
            fi
        else
            echo "   ❌ 处理失败: $SOURCE_DIR"
            echo "$RESULT"
        fi
    else
        echo "   ⚠️  跳过不存在的目录: $SOURCE_DIR"
    fi
done

echo ""
echo "========================================="
echo "🎉 头文件生成完成!"
echo "📊 总计生成: $TOTAL_GENERATED 个 .generate.h 文件"

# 如果有生成文件，建议重新生成项目文件
if [ $TOTAL_GENERATED -gt 0 ]; then
    echo ""
    echo "💡 提示: 已生成新的头文件，建议运行以下命令重新生成IDE项目文件:"
    echo "   ./GenerateProjectFiles.sh"
fi

echo ""
