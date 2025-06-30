#!/bin/bash

echo "========================================="
echo "Nut 项目初始化脚本 (*nix)"
echo "========================================="

# 设置项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

echo "[1/4] 安装Python依赖..."
pip3 install -r BuildSystem/Requirements.txt

echo "[2/4] 检查C++编译器..."
if command -v g++ >/dev/null 2>&1; then
    echo "检测到g++编译器"
elif command -v clang++ >/dev/null 2>&1; then
    echo "检测到clang++编译器"
else
    echo "未检测到g++或clang++，请手动安装GCC或Clang！"
fi

echo "[3/4] 检查Protobuf编译器 (protoc)..."
if command -v protoc >/dev/null 2>&1; then
    echo "检测到Protobuf编译器 (protoc)"
else
    echo "未检测到protoc，正在自动下载安装..."
    ARCH=$(uname -m)
    OS=$(uname -s)
    PROTOC_VERSION=21.12
    if [ "$OS" = "Linux" ]; then
        if [ "$ARCH" = "x86_64" ]; then
            PROTOC_URL="https://github.com/protocolbuffers/protobuf/releases/download/v$PROTOC_VERSION/protoc-$PROTOC_VERSION-linux-x86_64.zip"
        elif [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then
            PROTOC_URL="https://github.com/protocolbuffers/protobuf/releases/download/v$PROTOC_VERSION/protoc-$PROTOC_VERSION-linux-aarch_64.zip"
        else
            echo "暂不支持的Linux架构: $ARCH，请手动安装Protobuf"
            exit 1
        fi
    elif [ "$OS" = "Darwin" ]; then
        if [ "$ARCH" = "x86_64" ]; then
            PROTOC_URL="https://github.com/protocolbuffers/protobuf/releases/download/v$PROTOC_VERSION/protoc-$PROTOC_VERSION-osx-x86_64.zip"
        elif [ "$ARCH" = "arm64" ]; then
            PROTOC_URL="https://github.com/protocolbuffers/protobuf/releases/download/v$PROTOC_VERSION/protoc-$PROTOC_VERSION-osx-aarch_64.zip"
        else
            echo "暂不支持的macOS架构: $ARCH，请手动安装Protobuf"
            exit 1
        fi
    else
        echo "暂不支持的操作系统: $OS，请手动安装Protobuf"
        exit 1
    fi
    echo "下载: $PROTOC_URL"
    wget -q "$PROTOC_URL" -O protoc.zip
    unzip -o protoc.zip -d Tools/protoc
    if command -v protoc >/dev/null 2>&1; then
        echo "Protobuf (protoc) 安装成功"
    else
        echo "Protobuf (protoc) 安装失败，请手动安装"
    fi
fi

echo "[4/4] 安装Python protobuf包..."
python -m pipx install protobuf

echo
echo "初始化完成！"
