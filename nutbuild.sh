#!/bin/bash
# NutBuildTools 快捷调用脚本
# 使用方法: ./nutbuild.sh --target TestTarget --platform Windows --configuration Debug

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NUTBUILDTOOLS_DLL="$SCRIPT_DIR/Binary/NutBuildTools/NutBuildTools.dll"

if [ ! -f "$NUTBUILDTOOLS_DLL" ]; then
    echo "错误: 未找到 NutBuildTools.dll"
    echo "请确保已构建 NutBuildTools 项目"
    exit 1
fi

# 传递所有参数给 NutBuildTools
dotnet "$NUTBUILDTOOLS_DLL" "$@"