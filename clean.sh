#!/bin/bash

echo "=== Nut 项目清理工具 ==="
echo "正在清理所有二进制文件和构建产物..."

# 设置项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

# 清理 Binary 目录
echo "清理 Binary 目录..."
if [ -d "Binary" ]; then
    rm -rf Binary/*
    echo "  ✓ Binary 目录已清理"
else
    echo "  - Binary 目录不存在，跳过"
fi

# 清理 NutBuildTools 编译产物
echo "清理 NutBuildTools 编译产物..."
if [ -d "Source/Programs/NutBuildTools/bin" ]; then
    rm -rf Source/Programs/NutBuildTools/bin
    echo "  ✓ NutBuildTools/bin 已删除"
fi

if [ -d "Source/Programs/NutBuildTools/obj" ]; then
    rm -rf Source/Programs/NutBuildTools/obj
    echo "  ✓ NutBuildTools/obj 已删除"
fi

# 清理 Projects 构建目录
echo "清理 Projects 构建目录..."
if [ -d "Projects/Runtime/build" ]; then
    rm -rf Projects/Runtime/build
    echo "  ✓ Projects/Runtime/build 已删除"
fi

# 清理其他可能的构建产物
echo "清理其他构建产物..."

# 查找并删除所有 .o, .obj 文件
find "$PROJECT_ROOT" -name "*.o" -o -name "*.obj" -type f | while read file; do
    rm -f "$file"
    echo "  ✓ 删除: $file"
done

# 查找并删除所有可执行文件（但排除脚本）
find "$PROJECT_ROOT" -type f -perm +111 ! -name "*.sh" ! -name "*.py" ! -name "*.bat" ! -path "*/.*" | while read file; do
    # 检查是否是二进制文件
    if file "$file" | grep -q "executable\|library\|shared object"; then
        rm -f "$file"
        echo "  ✓ 删除二进制文件: $file"
    fi
done

# 清理 Xcode 构建缓存
echo "清理 Xcode 构建缓存..."
find "$PROJECT_ROOT" -name "xcuserdata" -type d | while read dir; do
    rm -rf "$dir"
    echo "  ✓ 删除 Xcode 用户数据: $dir"
done

# 清理 .DS_Store 文件
find "$PROJECT_ROOT" -name ".DS_Store" -type f -delete
echo "  ✓ 清理 .DS_Store 文件"

echo ""
echo "=== 清理完成 ==="
echo "所有二进制文件和构建产物已被清理"
echo "现在可以测试 NutBuildTools 的完整构建流程"
echo ""
echo "建议执行："
echo "  1. 在 Xcode 中打开项目并构建"
echo "  2. 或者手动运行: dotnet run --project Source/Programs/NutBuildTools -- --target PlayerService --platform Mac --configuration Debug"