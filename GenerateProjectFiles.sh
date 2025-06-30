#!/bin/bash

echo "========================================"
echo "Nut 项目文件生成器 (*nix)"
echo "========================================"

# 设置项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

echo "正在生成项目文件..."

# 调用Python脚本生成项目文件
python3 BuildSystem/BuildScripts/ProjectFileGenerator.py

echo "项目文件生成完成！"
echo ""
echo "正在刷新IntelliSense..."

# 调用Python脚本刷新IntelliSense
python3 BuildSystem/BuildScripts/IntelliSenseRefresher.py

echo "IntelliSense刷新完成！"
echo ""
echo "所有操作已完成！" 