#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Nut IntelliSense刷新器
自动为所有 Source/Runtime 下的服务/模块生成 compile_commands.json 到各自 Meta 目录
"""

import os
import sys
import json
import platform
from pathlib import Path
from typing import Dict, List

class IntelliSenseRefresher:
    def __init__(self):
        self.project_root = Path(os.curdir)
        self.runtime_dir = self.project_root / "Source" / "Runtime"
        self.include_dir = self.project_root / "Include"

    def RefreshIntelliSense(self) -> bool:
        """刷新IntelliSense"""
        print("正在刷新IntelliSense...")
        # 遍历所有 Runtime 下的服务/模块
        for module_dir in self.runtime_dir.iterdir():
            if module_dir.is_dir():
                self.GenerateModuleCompileCommands(module_dir)
        print("IntelliSense刷新完成!")
        return True

    def GenerateModuleCompileCommands(self, module_dir: Path):
        """为单个模块生成 compile_commands.json"""
        module_name = module_dir.name
        sources_dir = module_dir / "Sources"
        meta_dir = module_dir / "Meta"
        if not sources_dir.exists() or not meta_dir.exists():
            # 尝试在子目录中查找
            for sub_module_dir in module_dir.iterdir():
                if sub_module_dir.is_dir():
                    self.GenerateModuleCompileCommands(sub_module_dir)
            return
        compile_commands = []
        compiler = self.GetCompilerPath()
        for cpp_file in sources_dir.glob("*.cpp"):
            include_dirs = [str(sources_dir)]
            if self.include_dir.exists():
                include_dirs.append(str(self.include_dir))
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
        compile_commands_file = meta_dir / "compile_commands.json"
        compile_commands_file.write_text(json.dumps(compile_commands, indent=2, ensure_ascii=False), encoding='utf-8')
        print(f"  - 生成 {module_name}/Meta/compile_commands.json")

    def GetCompilerPath(self) -> str:
        """获取编译器路径"""
        if platform.system() == "Windows":
            possible_compilers = [
                "g++.exe",
                "C:/MinGW/bin/g++.exe",
                "C:/msys64/mingw64/bin/g++.exe",
                "cl.exe"
            ]
        else:
            possible_compilers = [
                "g++",
                "clang++",
                "/usr/bin/g++",
                "/usr/bin/clang++"
            ]
        for compiler in possible_compilers:
            try:
                import subprocess
                result = subprocess.run([compiler, "--version"], capture_output=True, text=True, check=True)
                if result.returncode == 0:
                    return compiler
            except (subprocess.CalledProcessError, FileNotFoundError):
                continue
        return "g++" if platform.system() != "Windows" else "g++.exe"

def Main():
    refresher = IntelliSenseRefresher()
    success = refresher.RefreshIntelliSense()
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(Main()) 