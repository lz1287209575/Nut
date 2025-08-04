#!/bin/bash

# GenerateHeaders.sh - 运行NutHeaderTools生成.generate.h文件
# 集成到Nut引擎构建流程中

set -e

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# 定义路径
HEADER_TOOL_PROJECT="$PROJECT_ROOT/Source/Programs/NutHeaderTools/NutHeaderTools.csproj"

# 默认参数
LOG_LEVEL="Info"
VERBOSE_MODE=false
FORCE_REGENERATE=false

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose|-v)
            VERBOSE_MODE=true
            LOG_LEVEL="Debug"
            shift
            ;;
        --quiet|-q)
            LOG_LEVEL="Warning"
            shift
            ;;
        --force|-f)
            FORCE_REGENERATE=true
            shift
            ;;
        --log-level)
            LOG_LEVEL="$2"
            shift 2
            ;;
        --help|-h)
            echo "用法: $0 [选项]"
            echo ""
            echo "选项:"
            echo "  --verbose, -v      启用详细输出"
            echo "  --quiet, -q        静默模式，只显示警告和错误"
            echo "  --force, -f        强制重新生成所有文件"
            echo "  --log-level LEVEL  设置日志级别 (Debug/Info/Warning/Error/Fatal)"
            echo "  --help, -h         显示此帮助信息"
            echo ""
            echo "说明:"
            echo "  该脚本使用Meta模式自动发现所有构建目标并生成对应的.generate.h文件"
            echo ""
            echo "示例:"
            echo "  $0                 使用默认设置运行"
            echo "  $0 --verbose       启用详细输出"
            echo "  $0 --force         强制重新生成所有文件"
            exit 0
            ;;
        *)
            echo "未知选项: $1"
            echo "使用 --help 查看可用选项"
            exit 1
            ;;
    esac
done

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

# 显示运行配置
echo "🔧 运行配置:"
echo "   日志级别: $LOG_LEVEL"
echo "   Meta模式: 启用（自动发现构建目标）"
echo "   强制重新生成: $FORCE_REGENERATE"
echo "   详细输出: $VERBOSE_MODE"

# 构建NutHeaderTools命令行参数
HEADER_TOOL_ARGS=()
HEADER_TOOL_ARGS+=("--log-level" "$LOG_LEVEL")
HEADER_TOOL_ARGS+=("--meta")  # 始终使用Meta模式

if [ "$VERBOSE_MODE" = true ]; then
    HEADER_TOOL_ARGS+=("--verbose")
fi

if [ "$FORCE_REGENERATE" = true ]; then
    HEADER_TOOL_ARGS+=("--force")
fi

# 运行代码生成
echo ""
echo "🔍 开始生成头文件..."
echo "📋 使用Meta模式自动发现所有构建目标"

if [ "$VERBOSE_MODE" = true ]; then
    echo "🚀 运行命令: dotnet run --project \"$HEADER_TOOL_PROJECT\" --configuration Release -- ${HEADER_TOOL_ARGS[*]}"
fi

RESULT=$(dotnet run --project "$HEADER_TOOL_PROJECT" --configuration Release -- "${HEADER_TOOL_ARGS[@]}" 2>&1)
EXIT_CODE=$?

if [ "$VERBOSE_MODE" = true ]; then
    echo ""
    echo "📝 详细输出:"
    echo "$RESULT"
    echo ""
fi

if [ $EXIT_CODE -eq 0 ]; then
    # 从输出中提取生成的文件数量
    TOTAL_GENERATED=$(echo "$RESULT" | grep -o "成功生成 [0-9]* 个" | grep -o "[0-9]*" || echo "0")
    
    if [ "$TOTAL_GENERATED" -gt 0 ]; then
        echo "✅ 成功生成 $TOTAL_GENERATED 个 .generate.h 文件"
    else
        echo "ℹ️  运行完成，无需生成新文件"
    fi
else
    echo "❌ NutHeaderTools运行失败"
    if [ "$VERBOSE_MODE" = false ]; then
        echo ""
        echo "错误输出:"
        echo "$RESULT"
    fi
    exit 1
fi

echo ""
echo "========================================="
echo "🎉 头文件生成完成!"

# 如果有生成文件，建议重新生成项目文件
if [ "$TOTAL_GENERATED" -gt 0 ]; then
    echo "📊 总计生成: $TOTAL_GENERATED 个 .generate.h 文件"
    echo ""
    echo "💡 提示: 已生成新的头文件，建议运行以下命令重新生成IDE项目文件:"
    echo "   ./GenerateProjectFiles.sh"
else
    echo "📊 无新文件生成"
fi

echo ""
