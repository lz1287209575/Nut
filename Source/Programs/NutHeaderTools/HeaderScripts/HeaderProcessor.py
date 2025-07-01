#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
NutHeaderTools - 头文件处理工具
用于分析和处理C++头文件，包括依赖分析、包含优化等
"""

import os
import sys
import re
from typing import List, Dict, Set, Optional
from pathlib import Path


class HeaderProcessor:
    """头文件处理器主类"""
    
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.header_files: Set[Path] = set()
        self.dependencies: Dict[Path, Set[Path]] = {}
        self.include_paths: List[Path] = []
        
    def scan_headers(self, directories: List[str] = None) -> Set[Path]:
        """扫描项目中的头文件"""
        if directories is None:
            directories = ['LibNut', 'MicroServices', 'ServiceAllocate']
            
        for dir_name in directories:
            dir_path = self.project_root / dir_name
            if dir_path.exists():
                for header_file in dir_path.rglob("*.h"):
                    self.header_files.add(header_file)
                for header_file in dir_path.rglob("*.hpp"):
                    self.header_files.add(header_file)
                    
        return self.header_files
    
    def analyze_dependencies(self) -> Dict[Path, Set[Path]]:
        """分析头文件依赖关系"""
        for header_file in self.header_files:
            self.dependencies[header_file] = self._extract_includes(header_file)
        return self.dependencies
    
    def _extract_includes(self, header_file: Path) -> Set[Path]:
        """提取头文件中的include语句"""
        includes = set()
        try:
            with open(header_file, 'r', encoding='utf-8') as f:
                content = f.read()
                
            # 匹配 #include 语句
            include_pattern = r'#include\s*[<"]([^>"]+)[>"]'
            matches = re.findall(include_pattern, content)
            
            for match in matches:
                # 尝试解析包含路径
                include_path = self._resolve_include_path(header_file, match)
                if include_path:
                    includes.add(include_path)
                    
        except Exception as e:
            print(f"警告: 无法读取文件 {header_file}: {e}")
            
        return includes
    
    def _resolve_include_path(self, current_file: Path, include_name: str) -> Optional[Path]:
        """解析include路径"""
        # 相对路径包含
        if include_name.startswith('./') or include_name.startswith('../'):
            resolved_path = (current_file.parent / include_name).resolve()
            if resolved_path.exists():
                return resolved_path
                
        # 系统包含路径
        for include_path in self.include_paths:
            resolved_path = include_path / include_name
            if resolved_path.exists():
                return resolved_path
                
        # 项目内包含
        for header_file in self.header_files:
            if header_file.name == include_name:
                return header_file
                
        return None
    
    def optimize_includes(self, header_file: Path) -> List[str]:
        """优化头文件的include语句"""
        # 这里可以实现include优化逻辑
        # 比如移除未使用的include，重新排序等
        return []
    
    def generate_dependency_graph(self, output_file: str = "header_dependencies.dot"):
        """生成依赖关系图"""
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write("digraph HeaderDependencies {\n")
            for header, deps in self.dependencies.items():
                for dep in deps:
                    f.write(f'    "{header.name}" -> "{dep.name}";\n')
            f.write("}\n")
    
    def find_circular_dependencies(self) -> List[List[Path]]:
        """查找循环依赖"""
        # 这里可以实现循环依赖检测算法
        return []
    
    def validate_headers(self) -> Dict[Path, List[str]]:
        """验证头文件的有效性"""
        issues = {}
        for header_file in self.header_files:
            issues[header_file] = self._validate_single_header(header_file)
        return issues
    
    def _validate_single_header(self, header_file: Path) -> List[str]:
        """验证单个头文件"""
        issues = []
        try:
            with open(header_file, 'r', encoding='utf-8') as f:
                content = f.read()
                
            # 检查头文件保护
            if not self._has_header_guard(content):
                issues.append("缺少头文件保护")
                
            # 检查其他常见问题
            if content.count('#include') > 20:
                issues.append("包含文件过多")
                
        except Exception as e:
            issues.append(f"读取文件失败: {e}")
            
        return issues
    
    def _has_header_guard(self, content: str) -> bool:
        """检查是否有头文件保护"""
        # 检查常见的头文件保护模式
        patterns = [
            r'#ifndef\s+\w+\s*\n#define\s+\w+',
            r'#pragma\s+once'
        ]
        
        for pattern in patterns:
            if re.search(pattern, content, re.MULTILINE):
                return True
        return False


def main():
    """主函数"""
    if len(sys.argv) < 2:
        print("用法: python HeaderProcessor.py <项目根目录>")
        sys.exit(1)
        
    project_root = sys.argv[1]
    processor = HeaderProcessor(project_root)
    
    print("扫描头文件...")
    headers = processor.scan_headers()
    print(f"找到 {len(headers)} 个头文件")
    
    print("分析依赖关系...")
    dependencies = processor.analyze_dependencies()
    
    print("验证头文件...")
    issues = processor.validate_headers()
    
    for header, header_issues in issues.items():
        if header_issues:
            print(f"\n{header}:")
            for issue in header_issues:
                print(f"  - {issue}")


if __name__ == "__main__":
    main() 