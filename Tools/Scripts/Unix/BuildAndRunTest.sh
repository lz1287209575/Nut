#!/bin/bash

# BuildAndRunTest.sh - 编译并运行NCLASS测试程序

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "🔧 构建并运行 NCLASS 测试程序"
echo "======================================="

# 设置路径
SOURCE_FILE="$PROJECT_ROOT/Source/Programs/TestNClass/MinimalTest.cpp"
BINARY_DIR="$PROJECT_ROOT/Binary"
OUTPUT_FILE="$BINARY_DIR/TestNClass"

# 检查源文件是否存在
if [ ! -f "$SOURCE_FILE" ]; then
    echo "❌ 错误: 源文件不存在: $SOURCE_FILE"
    exit 1
fi

# 确保二进制目录存在
mkdir -p "$BINARY_DIR"

echo "📁 源文件: $(basename "$SOURCE_FILE")"
echo "📁 输出文件: $(basename "$OUTPUT_FILE")"

# 编译命令
echo "🏗️  开始编译..."

clang++ -std=c++20 \
    -O0 -g -DDEBUG \
    -Wall -Wextra \
    -mmacosx-version-min=10.15 \
    "$SOURCE_FILE" \
    -o "$OUTPUT_FILE"

if [ $? -eq 0 ]; then
    echo "✅ 编译成功!"
    echo ""
    echo "🚀 运行测试程序..."
    echo "======================================="
    
    # 运行程序
    "$OUTPUT_FILE"
    
    echo ""
    echo "======================================="
    echo "🎉 测试完成!"
else
    echo "❌ 编译失败"
    exit 1
fi