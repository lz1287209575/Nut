#!/usr/bin/env python3
"""
clangd 配置生成器

为 Nut 项目生成 .clangd 配置文件和 compile_commands.json，
提供完整的代码补全和语法检查支持。
"""

import json
import logging
import os
import subprocess
from pathlib import Path
from typing import List, Dict, Set, Any

from Tools.ProjectGenerator.core.project_info import ProjectInfo

logger = logging.getLogger(__name__)


class ClangdGenerator:
    """clangd 配置生成器"""
    
    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.output_dir = project_root
        
    def GenerateClangdConfigs(self, projects: List[ProjectInfo]) -> Dict[str, Path]:
        """
        生成 clangd 相关配置文件
        
        Returns:
            生成的配置文件路径字典
        """
        generated_files = {}
        
        try:
            # 1. 生成全局 .clangd 配置文件
            clangd_config_path = self.GenerateGlobalClangdConfig()
            if clangd_config_path:
                generated_files['clangd_config'] = clangd_config_path
            
            # 2. 生成 compile_commands.json
            compile_commands_path = self.GenerateCompileCommands(projects)
            if compile_commands_path:
                generated_files['compile_commands'] = compile_commands_path
            
            # 3. 为每个 C++ 项目生成单独的配置
            for project in projects:
                if not project.is_csharp:  # 只处理 C++ 项目
                    project_config_path = self.GenerateProjectSpecificConfig(project)
                    if project_config_path:
                        generated_files[f'{project.name}_config'] = project_config_path
            
            logger.info(f"成功生成 {len(generated_files)} 个 clangd 配置文件")
            return generated_files
            
        except Exception as e:
            logger.error(f"生成 clangd 配置文件失败: {e}")
            return {}
    
    def GenerateGlobalClangdConfig(self) -> Path:
        """生成全局 .clangd 配置文件"""
        config_path = self.project_root / ".clangd"
        
        # 收集所有包含目录
        include_dirs = self.CollectGlobalIncludeDirs()
        
        # 生成配置内容
        config_content = self.BuildClangdConfig(include_dirs)
        
        try:
            with open(config_path, 'w', encoding='utf-8') as f:
                f.write(config_content)
            
            logger.info(f"生成全局 clangd 配置: {config_path}")
            return config_path
            
        except Exception as e:
            logger.error(f"写入 .clangd 配置文件失败: {e}")
            return None
    
    def GenerateCompileCommands(self, projects: List[ProjectInfo]) -> Path:
        """生成 compile_commands.json"""
        compile_commands = []
        
        # 为每个 C++ 项目生成编译命令
        for project in projects:
            if not project.is_csharp:
                project_commands = self.GenerateProjectCompileCommands(project)
                compile_commands.extend(project_commands)
        
        # 写入文件
        compile_commands_path = self.project_root / "compile_commands.json"
        
        try:
            with open(compile_commands_path, 'w', encoding='utf-8') as f:
                json.dump(compile_commands, f, indent=2, ensure_ascii=False)
            
            logger.info(f"生成 compile_commands.json: {compile_commands_path}")
            logger.info(f"包含 {len(compile_commands)} 条编译命令")
            return compile_commands_path
            
        except Exception as e:
            logger.error(f"写入 compile_commands.json 失败: {e}")
            return None
    
    def GenerateProjectSpecificConfig(self, project: ProjectInfo) -> Path:
        """为特定项目生成 .clangd 配置"""
        project_dir = project.path
        config_path = project_dir / ".clangd"
        
        # 收集项目特定的包含目录
        include_dirs = self.CollectProjectIncludeDirs(project)
        
        # 生成配置内容
        config_content = self.BuildClangdConfig(include_dirs, project_specific=True)
        
        try:
            with open(config_path, 'w', encoding='utf-8') as f:
                f.write(config_content)
            
            logger.debug(f"生成项目特定 clangd 配置: {config_path}")
            return config_path
            
        except Exception as e:
            logger.error(f"写入项目 .clangd 配置文件失败: {e}")
            return None
    
    def CollectGlobalIncludeDirs(self) -> Set[str]:
        """收集全局包含目录"""
        include_dirs = set()
        
        # 添加常用的系统和第三方库目录
        common_dirs = [
            "Source",
            "Source/Runtime/LibNut/Sources",
            "ThirdParty/spdlog/include",
            "ThirdParty/tcmalloc/src",
            "Binary",  # 生成的头文件可能在这里
            "Intermediate/Generated",  # 生成的头文件根目录
            "Intermediate/Generated/Runtime/LibNut/Sources",  # NLib 生成的头文件
        ]
        
        for dir_name in common_dirs:
            dir_path = self.project_root / dir_name
            if dir_path.exists():
                include_dirs.add(str(dir_path))
        
        # 自动发现所有 Sources 目录
        sources_dirs = list(self.project_root.rglob("Sources"))
        for sources_dir in sources_dirs:
            if sources_dir.is_dir():
                include_dirs.add(str(sources_dir))
        
        # 自动发现所有 include 目录
        include_dirs_found = list(self.project_root.rglob("include"))
        for include_dir in include_dirs_found:
            if include_dir.is_dir():
                include_dirs.add(str(include_dir))
        
        # 自动发现 Intermediate/Generated 下的所有子目录
        generated_base = self.project_root / "Intermediate" / "Generated"
        if generated_base.exists():
            # 添加 Generated 根目录
            include_dirs.add(str(generated_base))
            
            # 递归添加所有子目录，特别是包含生成头文件的目录
            for generated_dir in generated_base.rglob("*"):
                if generated_dir.is_dir():
                    # 检查是否包含 .h 或 .generate.h 文件
                    has_headers = any(generated_dir.glob("*.h")) or any(generated_dir.glob("*.generate.h"))
                    if has_headers:
                        include_dirs.add(str(generated_dir))
                        logger.debug(f"添加生成头文件目录: {generated_dir}")
        
        return include_dirs
    
    def CollectProjectIncludeDirs(self, project: ProjectInfo) -> Set[str]:
        """收集项目特定的包含目录"""
        include_dirs = set()
        
        # 添加项目自己的源目录
        project_sources_dir = project.path / "Sources"
        if project_sources_dir.exists():
            include_dirs.add(str(project_sources_dir))
        
        # 添加项目特定的生成头文件目录
        project_relative_path = project.path.relative_to(self.project_root)
        project_generated_dir = self.project_root / "Intermediate" / "Generated" / project_relative_path / "Sources"
        if project_generated_dir.exists():
            include_dirs.add(str(project_generated_dir))
            logger.debug(f"添加项目 {project.name} 的生成头文件目录: {project_generated_dir}")
        
        # 添加全局包含目录
        include_dirs.update(self.CollectGlobalIncludeDirs())
        
        return include_dirs
    
    def GenerateProjectCompileCommands(self, project: ProjectInfo) -> List[Dict[str, Any]]:
        """为项目生成编译命令"""
        commands = []
        
        # 收集项目源文件 - 使用项目文件中的源文件信息
        source_files = []
        
        # 从项目的文件列表中获取源文件
        if hasattr(project, 'files'):
            # project.files is a dict with FileGroup keys and List[FileInfo] values
            source_file_infos = project.GetSourceFiles()  # This returns List[FileInfo]
            for file_info in source_file_infos:
                if file_info.path.suffix in ['.cpp', '.cxx', '.cc', '.c']:
                    source_files.append(file_info.path)
        
        # 如果没有找到文件，尝试直接搜索 Sources 目录和项目目录
        if not source_files:
            # 首先尝试 Sources 子目录
            sources_dir = project.path / "Sources"
            if sources_dir.exists():
                cpp_extensions = ['.cpp', '.cxx', '.cc', '.c']
                for ext in cpp_extensions:
                    source_files.extend(sources_dir.rglob(f"*{ext}"))
            
            # 如果 Sources 目录不存在，尝试在项目根目录中查找
            if not source_files:
                project_dir = project.path
                cpp_extensions = ['.cpp', '.cxx', '.cc', '.c']
                for ext in cpp_extensions:
                    source_files.extend(project_dir.glob(f"*{ext}"))
        
        logger.debug(f"项目 {project.name} 找到 {len(source_files)} 个源文件")
        
        if not source_files:
            return commands
        
        # 收集包含目录
        include_dirs = self.CollectProjectIncludeDirs(project)
        
        # 构建编译器参数
        compiler_args = self.BuildCompilerArgs(include_dirs)
        
        # 为每个源文件生成编译命令
        for source_file in source_files:
            command = {
                "directory": str(self.project_root),
                "file": str(source_file),
                "command": f"clang++ {' '.join(compiler_args)} -c {source_file}",
                "arguments": ["clang++"] + compiler_args + ["-c", str(source_file)]
            }
            commands.append(command)
        
        return commands
    
    def BuildCompilerArgs(self, include_dirs: Set[str]) -> List[str]:
        """构建编译器参数"""
        args = []
        
        # C++ 标准
        args.extend(["-std=c++20"])
        
        # 编译器警告
        args.extend(["-Wall", "-Wextra"])
        
        # 调试信息
        args.extend(["-g", "-O0"])
        
        # 定义宏
        args.extend(["-DDEBUG"])
        
        # 平台特定设置
        import platform
        if platform.system() == "Darwin":
            args.extend(["-mmacosx-version-min=10.15"])
        
        # 包含目录
        for include_dir in sorted(include_dirs):
            args.extend(["-I", include_dir])
        
        return args
    
    def BuildClangdConfig(self, include_dirs: Set[str], project_specific: bool = False) -> str:
        """构建 .clangd 配置文件内容"""
        
        # 构建编译器参数
        compiler_args = self.BuildCompilerArgs(include_dirs)
        
        config = {
            "CompileFlags": {
                "Add": compiler_args,
                "Remove": [
                    # 移除一些可能导致问题的参数
                    "-W*",  # 移除所有警告设置，让 clangd 自己处理
                ]
            },
            "Index": {
                "Background": "Build",  # 后台建立索引
                "StandardLibrary": True  # 索引标准库
            },
            "InlayHints": {
                "Enabled": True,
                "ParameterNames": True,
                "DeducedTypes": True
            },
            "Hover": {
                "ShowAKA": True
            },
            "Completion": {
                "AllScopes": True
            }
        }
        
        if project_specific:
            # 项目特定的配置可以更激进一些
            config["Diagnostics"] = {
                "ClangTidy": {
                    "Add": [
                        "modernize-*",
                        "readability-*",
                        "performance-*"
                    ],
                    "Remove": [
                        "modernize-use-trailing-return-type"
                    ]
                }
            }
        
        # 转换为 YAML 格式
        yaml_content = self.DictToYaml(config)
        
        return f"""# clangd 配置文件
# 由 Nut 项目生成器自动生成 - 请勿手动修改

{yaml_content}
"""
    
    def DictToYaml(self, data: Any, indent: int = 0) -> str:
        """简单的字典到 YAML 转换"""
        lines = []
        indent_str = "  " * indent
        
        if isinstance(data, dict):
            for key, value in data.items():
                if isinstance(value, (dict, list)):
                    lines.append(f"{indent_str}{key}:")
                    lines.append(self.DictToYaml(value, indent + 1))
                else:
                    lines.append(f"{indent_str}{key}: {self.FormatYamlValue(value)}")
        elif isinstance(data, list):
            for item in data:
                if isinstance(item, (dict, list)):
                    lines.append(f"{indent_str}-")
                    lines.append(self.DictToYaml(item, indent + 1))
                else:
                    lines.append(f"{indent_str}- {self.FormatYamlValue(item)}")
        
        return "\n".join(lines)
    
    def FormatYamlValue(self, value: Any) -> str:
        """格式化 YAML 值"""
        if isinstance(value, bool):
            return "true" if value else "false"
        elif isinstance(value, str):
            # 如果字符串包含特殊字符，加引号
            if any(char in value for char in [" ", ":", "-", "*"]):
                return f'"{value}"'
            return value
        else:
            return str(value)