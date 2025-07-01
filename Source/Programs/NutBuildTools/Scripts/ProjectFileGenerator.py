#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Nut 项目文件生成器
生成IDE项目文件、构建配置等
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

class ProjectFileGenerator:
    def __init__(self):
        self.project_root = Path(os.curdir)
        self.services_dir = self.project_root / "MicroServices"
        self.build_dir = self.project_root / "Build"
        
    def GenerateProjectFiles(self) -> bool:
        """生成项目文件"""
        print("正在生成项目文件...")
        
        # 创建构建目录
        self.build_dir.mkdir(exist_ok=True)
        
        # 生成VSCode配置
        self.GenerateVSCodeConfig()
        
        print("项目文件生成完成!")
        return True
    
    def GenerateVSCodeConfig(self):
        """生成VSCode配置"""
        vscode_dir = self.project_root / ".vscode"
        vscode_dir.mkdir(exist_ok=True)
        
        # c_cpp_properties.json
        cpp_properties = {
            "configurations": [
                {
                    "name": "Win32" if platform.system() == "Windows" else "Linux",
                    "includePath": [
                        "${workspaceFolder}/**",
                        "${workspaceFolder}/Include",
                        "${workspaceFolder}/MicroServices/*/Sources",
                        "${workspaceFolder}/MicroServices/*/Protos"
                    ],
                    "defines": [],
                    "compilerPath": "/usr/bin/g++" if platform.system() != "Windows" else "C:/MinGW/bin/g++.exe",
                    "cStandard": "c17",
                    "cppStandard": "c++20",
                    "intelliSenseMode": "linux-gcc-x64" if platform.system() != "Windows" else "windows-gcc-x64",
                    "compileCommands": "${workspaceFolder}/compile_commands.json"
                }
            ],
            "version": 4
        }
        
        cpp_properties_file = vscode_dir / "c_cpp_properties.json"
        cpp_properties_file.write_text(json.dumps(cpp_properties, indent=2, ensure_ascii=False), encoding='utf-8')
        print("  - 生成 VSCode c_cpp_properties.json")
        
        # tasks.json
        tasks = {
            "version": "2.0.0",
            "tasks": [
                {
                    "label": "Generate Project Files",
                    "type": "shell",
                    "command": "python",
                    "args": ["Source/Programs/NutBuildTools/Scripts/ProjectFileGenerator.py"],
                    "group": "build",
                    "presentation": {
                        "echo": True,
                        "reveal": "always",
                        "focus": False,
                        "panel": "shared"
                    }
                },
                {
                    "label": "Refresh IntelliSense",
                    "type": "shell",
                    "command": "python",
                    "args": ["Source/Programs/NutBuildTools/Scripts/IntelliSenseRefresher.py"],
                    "group": "build",
                    "presentation": {
                        "echo": True,
                        "reveal": "always",
                        "focus": False,
                        "panel": "shared"
                    }
                },
                {
                    "label": "Build All",
                    "type": "shell",
                    "command": "python",
                    "args": ["Source/Programs/NutBuildTools/BuildTool.py", "build"],
                    "group": "build",
                    "presentation": {
                        "echo": True,
                        "reveal": "always",
                        "focus": False,
                        "panel": "shared"
                    }
                },
                {
                    "label": "Clean All",
                    "type": "shell",
                    "command": "python",
                    "args": ["Source/Programs/NutBuildTools/BuildTool.py", "clean"],
                    "group": "build",
                    "presentation": {
                        "echo": True,
                        "reveal": "always",
                        "focus": False,
                        "panel": "shared"
                    }
                },
            ]
        }
        
        tasks_file = vscode_dir / "tasks.json"
        tasks_file.write_text(json.dumps(tasks, indent=2, ensure_ascii=False), encoding='utf-8')
        print("  - 生成 VSCode tasks.json")


def Main():
    """主函数"""
    generator = ProjectFileGenerator()
    success = generator.GenerateProjectFiles()
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(Main()) 