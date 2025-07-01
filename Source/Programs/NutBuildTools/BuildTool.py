#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
NutBuildTools - 统一构建入口
"""
import sys
import argparse
from pathlib import Path

# 添加核心模块路径
core_dir = Path(__file__).parent / "Core"
sys.path.insert(0, str(core_dir))

from BuildEngine import BuildEngine

# 辅助脚本路径
scripts_dir = Path(__file__).parent / "Scripts"
sys.path.insert(0, str(scripts_dir))


def main():
    parser = argparse.ArgumentParser(description="NutBuildTools - 统一构建入口")
    subparsers = parser.add_subparsers(dest="command", help="可用命令")

    # build
    build_parser = subparsers.add_parser("build", help="编译指定模块或全部模块")
    build_parser.add_argument("module", nargs="?", help="模块名（可选）")

    # clean
    clean_parser = subparsers.add_parser("clean", help="清理指定模块或全部模块")
    clean_parser.add_argument("module", nargs="?", help="模块名（可选）")

    # list
    subparsers.add_parser("list", help="列出所有模块")

    # generate
    generate_parser = subparsers.add_parser("generate", help="生成新服务/模块")
    generate_parser.add_argument("name", help="服务/模块名")
    generate_parser.add_argument("--port", type=int, help="端口号（可选）")

    # projectfiles
    subparsers.add_parser("projectfiles", help="生成IDE项目文件")

    args = parser.parse_args()
    engine = BuildEngine()

    if args.command == "build":
        engine.Build(args.module)
    elif args.command == "clean":
        engine.Clean(args.module)
    elif args.command == "list":
        engine.ListServices()
    elif args.command == "generate":
        from ServiceGenerator import ServiceGenerator
        generator = ServiceGenerator()
        generator.GenerateService(args.name, args.port)
    elif args.command == "projectfiles":
        from ProjectFileGenerator import main as gen_proj
        gen_proj()
    else:
        parser.print_help()

if __name__ == "__main__":
    main() 