#!/bin/bash

echo "========================================"
echo "Nut IntelliSense刷新器 (*nix)"
echo "========================================"

# 设置项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

echo "正在刷新IntelliSense..."

# 调用Python脚本刷新IntelliSense
python3 BuildSystem/BuildScripts/IntelliSenseRefresher.py

echo "IntelliSense刷新完成！" 