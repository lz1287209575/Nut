#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Nut IntelliSense刷新器
自动为所有服务生成compile_commands.json到Meta目录
"""

import os
import sys
import json
import platform
from pathlib import Path
from typing import Dict, List

# 设置控制台输出编码
if platform.system() == "Windows":
    import codecs
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout.detach())
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr.detach())

class IntelliSenseRefresher:
    def __init__(self):
        self.project_root = Path(__file__).parent.parent.parent
        self.services_dir = self.project_root / "MicroServices"
        
    def RefreshIntelliSense(self) -> bool:
        """刷新IntelliSense"""
        print("正在刷新IntelliSense...")
        
        # 遍历所有服务
        for service_dir in self.services_dir.iterdir():
            if service_dir.is_dir():
                self.GenerateServiceCompileCommands(service_dir)
        
        # 生成全局compile_commands.json
        self.GenerateGlobalCompileCommands()
        
        print("IntelliSense刷新完成!")
        return True
    
    def GenerateServiceCompileCommands(self, service_dir: Path):
        """为单个服务生成compile_commands.json"""
        service_name = service_dir.name
        sources_dir = service_dir / "Sources"
        meta_dir = service_dir / "Meta"
        
        if not sources_dir.exists() or not meta_dir.exists():
            return
        
        compile_commands = []
        
        # 获取编译器路径
        compiler = self.GetCompilerPath()
        
        # 为每个源文件生成编译命令
        for cpp_file in sources_dir.glob("*.cpp"):
            # 构建包含目录
            include_dirs = [
                str(sources_dir),
                str(self.project_root / "Include")
            ]
            
            # 构建编译命令
            include_flags = []
            for include_dir in include_dirs:
                include_flags.extend(["-I", include_dir])
            
            command_parts = [
                compiler,
                "-std=c++20",
                "-c",
                str(cpp_file),
                "-o",
                f"{cpp_file.stem}.o"
            ]
            command_parts.extend(include_flags)
            
            command = {
                "directory": str(self.project_root),
                "command": " ".join(command_parts),
                "file": str(cpp_file)
            }
            compile_commands.append(command)
        
        # 写入服务的compile_commands.json
        compile_commands_file = meta_dir / "compile_commands.json"
        compile_commands_file.write_text(json.dumps(compile_commands, indent=2, ensure_ascii=False), encoding='utf-8')
        print(f"  - 生成 {service_name}/Meta/compile_commands.json")
    
    def GenerateGlobalCompileCommands(self):
        """生成全局compile_commands.json"""
        compile_commands = []
        
        # 遍历所有服务
        for service_dir in self.services_dir.iterdir():
            if service_dir.is_dir():
                sources_dir = service_dir / "Sources"
                if sources_dir.exists():
                    for cpp_file in sources_dir.glob("*.cpp"):
                        # 构建包含目录
                        include_dirs = [
                            str(sources_dir),
                            str(self.project_root / "Include")
                        ]
                        
                        # 构建编译命令
                        include_flags = []
                        for include_dir in include_dirs:
                            include_flags.extend(["-I", include_dir])
                        
                        compiler = self.GetCompilerPath()
                        command_parts = [
                            compiler,
                            "-std=c++20",
                            "-c",
                            str(cpp_file),
                            "-o",
                            f"{cpp_file.stem}.o"
                        ]
                        command_parts.extend(include_flags)
                        
                        command = {
                            "directory": str(self.project_root),
                            "command": " ".join(command_parts),
                            "file": str(cpp_file)
                        }
                        compile_commands.append(command)
        
        # 写入全局compile_commands.json
        global_compile_commands = self.project_root / "compile_commands.json"
        global_compile_commands.write_text(json.dumps(compile_commands, indent=2, ensure_ascii=False), encoding='utf-8')
        print("  - 生成全局 compile_commands.json")
    
    def GetCompilerPath(self) -> str:
        """获取编译器路径"""
        if platform.system() == "Windows":
            # Windows下尝试找到编译器
            possible_compilers = [
                "g++.exe",
                "C:/MinGW/bin/g++.exe",
                "C:/msys64/mingw64/bin/g++.exe",
                "cl.exe"  # MSVC
            ]
        else:
            # *nix下尝试找到编译器
            possible_compilers = [
                "g++",
                "clang++",
                "/usr/bin/g++",
                "/usr/bin/clang++"
            ]
        
        for compiler in possible_compilers:
            try:
                import subprocess
                result = subprocess.run([compiler, "--version"], 
                                      capture_output=True, text=True, check=True)
                if result.returncode == 0:
                    return compiler
            except (subprocess.CalledProcessError, FileNotFoundError):
                continue
        
        # 默认返回g++
        return "g++" if platform.system() != "Windows" else "g++.exe"

def Main():
    """主函数"""
    refresher = IntelliSenseRefresher()
    success = refresher.RefreshIntelliSense()
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(Main()) 