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
        self.project_root = Path(__file__).parent.parent.parent
        self.services_dir = self.project_root / "MicroServices"
        self.build_dir = self.project_root / "Build"
        
    def GenerateProjectFiles(self) -> bool:
        """生成项目文件"""
        print("正在生成项目文件...")
        
        # 创建构建目录
        self.build_dir.mkdir(exist_ok=True)
        
        # 生成CMakeLists.txt（可选，用于某些IDE）
        self.GenerateCMakeLists()
        
        # 生成VSCode配置
        self.GenerateVSCodeConfig()
        
        # 生成CLion配置
        self.GenerateCLionConfig()
        
        # 生成全局compile_commands.json
        self.GenerateGlobalCompileCommands()
        
        print("项目文件生成完成!")
        return True
    
    def GenerateCMakeLists(self):
        """生成CMakeLists.txt"""
        cmake_content = f'''cmake_minimum_required(VERSION 3.20)
project(Nut VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找所有服务
file(GLOB_RECURSE SERVICE_DIRS "MicroServices/*/")
foreach(SERVICE_DIR ${{SERVICE_DIRS}})
    if(EXISTS "${{SERVICE_DIR}}/Meta")
        get_filename_component(SERVICE_NAME ${{SERVICE_DIR}} NAME)
        message(STATUS "Found service: ${{SERVICE_NAME}}")
    endif()
endforeach()

# 包含目录
include_directories(${{CMAKE_SOURCE_DIR}}/Include)
include_directories(${{CMAKE_SOURCE_DIR}}/MicroServices/*/Sources)
'''
        
        cmake_file = self.project_root / "CMakeLists.txt"
        cmake_file.write_text(cmake_content, encoding='utf-8')
        print("  - 生成 CMakeLists.txt")
    
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
                    "command": "python" if platform.system() == "Windows" else "python3",
                    "args": ["BuildSystem/BuildScripts/ProjectFileGenerator.py"],
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
                    "command": "python" if platform.system() == "Windows" else "python3",
                    "args": ["BuildSystem/BuildScripts/IntelliSenseRefresher.py"],
                    "group": "build",
                    "presentation": {
                        "echo": True,
                        "reveal": "always",
                        "focus": False,
                        "panel": "shared"
                    }
                }
            ]
        }
        
        tasks_file = vscode_dir / "tasks.json"
        tasks_file.write_text(json.dumps(tasks, indent=2, ensure_ascii=False), encoding='utf-8')
        print("  - 生成 VSCode tasks.json")
    
    def GenerateCLionConfig(self):
        """生成CLion配置"""
        clion_dir = self.project_root / ".idea"
        clion_dir.mkdir(exist_ok=True)
        
        # misc.xml
        misc_content = f'''<?xml version="1.0" encoding="UTF-8"?>
<project version="4">
  <component name="CMakeWorkspace" PROJECT_DIR="$PROJECT_DIR$" />
  <component name="ProjectRootManager" version="2" languageLevel="JDK_17" default="true" project-jdk-name="17" project-jdk-type="JavaSDK">
    <output url="file://$PROJECT_DIR$/out" />
  </component>
</project>'''
        
        misc_file = clion_dir / "misc.xml"
        misc_file.write_text(misc_content, encoding='utf-8')
        print("  - 生成 CLion misc.xml")
    
    def GenerateGlobalCompileCommands(self):
        """生成全局compile_commands.json"""
        compile_commands = []
        
        # 遍历所有服务
        for service_dir in self.services_dir.iterdir():
            if service_dir.is_dir():
                sources_dir = service_dir / "Sources"
                if sources_dir.exists():
                    for cpp_file in sources_dir.glob("*.cpp"):
                        command = {
                            "directory": str(self.project_root),
                            "command": f"g++ -std=c++20 -I{sources_dir} -c {cpp_file} -o {cpp_file.stem}.o",
                            "file": str(cpp_file)
                        }
                        compile_commands.append(command)
        
        # 写入全局compile_commands.json
        global_compile_commands = self.project_root / "compile_commands.json"
        global_compile_commands.write_text(json.dumps(compile_commands, indent=2, ensure_ascii=False), encoding='utf-8')
        print("  - 生成全局 compile_commands.json")

def Main():
    """主函数"""
    generator = ProjectFileGenerator()
    success = generator.GenerateProjectFiles()
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(Main()) 