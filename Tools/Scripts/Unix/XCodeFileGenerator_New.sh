#!/bin/bash

# Nut XCode 项目文件生成器 (Python 版本)
# 这个脚本调用工程化的 Python 项目生成器

set -e

PROJECT_ROOT=$(cd "$(dirname "$0")/../.."; pwd)
GENERATOR_SCRIPT="$PROJECT_ROOT/Tools/ProjectGenerator/tool.py"

echo "🚀 Nut XCode 项目文件生成器 (工程化版本)"
echo "📁 项目根目录: $PROJECT_ROOT"
echo ""

# 检查 Python 环境
if ! command -v python3 &> /dev/null; then
    echo "❌ 错误: 未找到 python3，请先安装 Python 3.6+"
    exit 1
fi

# 检查生成器脚本
if [ ! -f "$GENERATOR_SCRIPT" ]; then
    echo "❌ 错误: 未找到生成器脚本 $GENERATOR_SCRIPT"
    exit 1
fi

# 调用 Python 生成器
echo "🔧 调用 Python 项目生成器..."
python3 "$GENERATOR_SCRIPT" xcode --project-root "$PROJECT_ROOT" "$@"

echo ""
echo "✅ XCode 项目文件生成完成！"
echo ""
echo "生成的文件："
echo "  📁 Projects/ - 各个项目的 .xcodeproj 文件"
echo "  🗂️ Nut.xcworkspace - 主工作空间文件"
echo ""
echo "使用方法："
echo "  1. 使用 Xcode 打开 Nut.xcworkspace"
echo "  2. 或者直接打开单个项目的 .xcodeproj 文件"