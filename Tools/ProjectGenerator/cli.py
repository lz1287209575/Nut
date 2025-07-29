#!/usr/bin/env python3
"""
Nut 项目文件生成器命令行接口

工程化的项目文件生成工具，支持 XCode、Visual Studio 等多种 IDE 格式
"""

import argparse
import logging
import sys
from pathlib import Path
from typing import List

from Tools.ProjectGenerator.core.project_discoverer import ProjectDiscoverer
from Tools.ProjectGenerator.generators.xcode_generator import XCodeProjectGenerator
from Tools.ProjectGenerator.generators.vcxproj_generator import VcxprojGenerator
from Tools.ProjectGenerator.generators.workspace_generator import WorkspaceGenerator
from Tools.ProjectGenerator.generators.clangd_generator import ClangdGenerator


def SetupLogging(verbose: bool = False):
    """配置日志"""
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(
        level=level,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        datefmt='%H:%M:%S'
    )


def DiscoverProjects(project_root: Path) -> List:
    """发现所有项目"""
    discoverer = ProjectDiscoverer(project_root)
    projects = discoverer.DiscoverAllProjects()
    
    if not projects:
        print("❌ 未发现任何项目")
        return []
    
    # 按分组显示项目
    print(f"📁 发现 {len(projects)} 个项目:")
    current_group = None
    
    for project in sorted(projects, key=lambda p: (p.group_name, p.name)):
        if project.group_name != current_group:
            print(f"  📂 {project.group_name}:")
            current_group = project.group_name
        
        project_type_icon = "⚙️" if project.is_csharp else "🔧"
        print(f"    {project_type_icon} {project.name} ({project.project_type.value})")
    
    return projects


def GenerateXcodeProjects(projects: List, project_root: Path):
    """生成 XCode 项目文件"""
    print("\n🍎 生成 XCode 项目文件...")
    
    generator = XCodeProjectGenerator(project_root)
    generated_files = generator.GenerateProjects(projects)
    
    # 生成工作空间
    workspace_generator = WorkspaceGenerator(project_root)
    workspace_path = workspace_generator.GenerateWorkspace(projects, "xcode")
    
    print(f"\n✅ XCode 项目生成完成:")
    for file_path in generated_files:
        if file_path:  # 跳过 None 值（C# 项目）
            print(f"  📁 {file_path.relative_to(project_root)}")
    
    if workspace_path:
        print(f"  🗂️ {workspace_path.relative_to(project_root)}")


def GenerateVsProjects(projects: List, project_root: Path):
    """生成 Visual Studio 项目文件"""
    print("\n🔵 生成 Visual Studio 项目文件...")
    
    generator = VcxprojGenerator(project_root)
    generated_files = generator.GenerateProjects(projects)
    
    # 生成解决方案
    workspace_generator = WorkspaceGenerator(project_root)
    solution_path = workspace_generator.GenerateWorkspace(projects, "vs")
    
    print(f"\n✅ Visual Studio 项目生成完成:")
    for file_path in generated_files:
        if file_path:  # 跳过 None 值（C# 项目已有 .csproj）
            print(f"  📁 {file_path.relative_to(project_root)}")
    
    if solution_path:
        print(f"  📄 {solution_path.relative_to(project_root)}")


def GenerateClangdConfigs(projects: List, project_root: Path):
    """生成 clangd 配置文件"""
    print("\n🔧 生成 clangd 配置文件...")
    
    generator = ClangdGenerator(project_root)
    generated_files = generator.GenerateClangdConfigs(projects)
    
    print(f"\n✅ clangd 配置生成完成:")
    for config_name, file_path in generated_files.items():
        if file_path:
            print(f"  📄 {file_path.relative_to(project_root)}")
    
    if generated_files:
        print("\n💡 clangd 配置说明:")
        print("  📖 .clangd - 全局 clangd 配置文件")
        print("  📖 compile_commands.json - 编译命令数据库")
        print("  📖 各项目目录下的 .clangd - 项目特定配置")
        print("  🚀 重启 VS Code/编辑器以应用新配置")


def GenerateAllProjects(projects: List, project_root: Path):
    """生成所有格式的项目文件"""
    import platform
    
    current_platform = platform.system().lower()
    print(f"\n🚀 生成适合当前平台 ({current_platform}) 的项目文件...")
    
    if current_platform == "windows":
        # Windows 平台只生成 Visual Studio 项目
        print("🔵 Windows 平台：仅生成 Visual Studio 项目")
        GenerateVsProjects(projects, project_root)
    elif current_platform == "darwin":
        # macOS 平台生成 XCode 和 Visual Studio 项目
        print("🍎 macOS 平台：生成 XCode 和 Visual Studio 项目")
        GenerateXcodeProjects(projects, project_root)
        GenerateVsProjects(projects, project_root)
    else:
        # Linux 或其他平台只生成 Visual Studio 项目（可以用 VS Code 打开）
        print("🐧 Linux/其他平台：仅生成 Visual Studio 项目")
        GenerateVsProjects(projects, project_root)
    
    # 所有平台都生成 clangd 配置
    GenerateClangdConfigs(projects, project_root)


def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description="Nut 项目文件生成器",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s generate          # 生成所有格式的项目文件
  %(prog)s xcode             # 仅生成 XCode 项目文件  
  %(prog)s vs                # 仅生成 Visual Studio 项目文件
  %(prog)s clangd            # 仅生成 clangd 配置文件
  %(prog)s discover          # 仅发现项目，不生成文件
  %(prog)s --verbose xcode   # 详细输出模式
        """
    )
    
    parser.add_argument(
        'command',
        choices=['discover', 'xcode', 'vs', 'clangd', 'generate'],
        help='执行的命令'
    )
    
    parser.add_argument(
        '--project-root',
        type=Path,
        default=Path.cwd(),
        help='项目根目录 (默认: 当前目录)'
    )
    
    parser.add_argument(
        '--verbose', '-v', 
        action='store_true',
        help='详细输出模式'
    )
    
    args = parser.parse_args()
    
    # 配置日志
    SetupLogging(args.verbose)
    
    # 检查项目根目录
    project_root = args.project_root.resolve()
    if not project_root.exists():
        print(f"❌ 项目根目录不存在: {project_root}")
        sys.exit(1)
    
    source_dir = project_root / "Source"
    if not source_dir.exists():
        print(f"❌ 源代码目录不存在: {source_dir}")
        sys.exit(1)
    
    print(f"🚀 Nut 项目文件生成器")
    print(f"📁 项目根目录: {project_root}")
    
    try:
        # 发现项目
        projects = DiscoverProjects(project_root)
        if not projects:
            sys.exit(1)
        
        # 执行相应命令
        if args.command == 'discover':
            # 仅发现项目，不生成文件
            pass
        elif args.command == 'xcode':
            GenerateXcodeProjects(projects, project_root)  
        elif args.command == 'vs':
            GenerateVsProjects(projects, project_root)
        elif args.command == 'clangd':
            GenerateClangdConfigs(projects, project_root)
        elif args.command == 'generate':
            GenerateAllProjects(projects, project_root)
        
        print(f"\n🎉 操作完成!")
        
    except Exception as e:
        logging.error(f"执行失败: {e}")
        if args.verbose:
            raise
        sys.exit(1)


if __name__ == '__main__':
    main()