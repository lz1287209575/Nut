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
python3 Source/Programs/NutBuildTools/Scripts/ProjectFileGenerator.py

echo "项目文件生成完成！"
echo ""

# 2. 刷新IntelliSense
echo "正在刷新IntelliSense..."
python3 Source/Programs/NutBuildTools/Scripts/IntelliSenseRefresher.py

echo "IntelliSense刷新完成！"
echo ""

echo ".clangd 已生成！"
echo ""
echo "所有操作已完成！" 