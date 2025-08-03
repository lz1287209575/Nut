#!/bin/bash

set -e

echo "========================================"
echo "Nut 项目文件生成器 (*nix)"
echo "========================================"

# 设置项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

# 1. 生成 NCLASS 反射头文件
echo "🔧 生成 NCLASS 反射头文件..."
if [ -f "./Tools/Scripts/Unix/GenerateHeaders.sh" ]; then
    bash ./Tools/Scripts/Unix/GenerateHeaders.sh
    echo "✅ NCLASS 头文件生成完成"
else
    echo "⚠️  未找到 GenerateHeaders.sh，跳过反射头文件生成"
fi

echo ""

# 2. 生成项目文件
echo "📁 生成项目文件..."
python3 Tools/ProjectGenerator/tool.py generate

echo ""
echo "========================================"
echo "🎉 所有操作已完成！"
echo ""
echo "生成的文件："
echo "  📁 ProjectFiles/ - IDE 项目文件"
echo "  📄 .clangd - clangd 全局配置"
echo "  📄 compile_commands.json - 编译命令数据库"
echo "  📄 各项目目录下的 .clangd - 项目特定配置"
echo ""
echo "💡 使用说明："
echo "  🍎 macOS: 打开 ProjectFiles/Nut.xcworkspace"
echo "  🔵 其他: 打开 ProjectFiles/Nut.sln"
echo "  🔧 VS Code: 直接打开项目根目录即可获得完整代码补全"
echo ""
