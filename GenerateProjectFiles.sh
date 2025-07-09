#!/bin/bash

set -e

echo "========================================"
echo "Nut 项目文件生成器 (*nix)"
echo "========================================"

# 设置项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

# 1. 生成项目文件
echo "正在生成项目文件..."
Tools/ProjectFileGenerator.sh generate

echo "项目文件生成完成！"
echo ""

# 2. 刷新IntelliSense
echo "正在刷新IntelliSense..."
if [ -f "Source/Programs/NutBuildTools/Scripts/IntelliSenseRefresher.py" ]; then
    python3 Source/Programs/NutBuildTools/Scripts/IntelliSenseRefresher.py
    echo "IntelliSense刷新完成！"
else
    echo "未找到 IntelliSenseRefresher.py，跳过 IntelliSense 刷新"
fi

echo ""
echo "生成的项目文件："
echo "  📁 Nut.sln - Visual Studio 解决方案"
echo "  📁 Nut.xcodeproj - Xcode 项目"
echo ""
echo "所有操作已完成！" 