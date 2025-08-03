#!/usr/bin/env python3
"""
NLib 命名规范检查脚本

检查代码是否符合新的命名规范：
- N*** - 继承自NObject的类
- C*** - 普通C++类
- I*** - 接口类
- E*** - 枚举类型
- T*** - 模板类
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple
from dataclasses import dataclass

@dataclass
class NamingIssue:
    file_path: Path
    line_number: int
    issue_type: str
    message: str
    suggestion: str

class NamingChecker:
    def __init__(self, source_dir: str):
        self.source_dir = Path(source_dir)
        self.issues: List[NamingIssue] = []
        
        # 命名规范模式
        self.patterns = {
            'nobject_class': r'class\s+(\w+)\s*:\s*public\s+NObject',
            'plain_class': r'class\s+(\w+)\s*(?!.*:.*NObject)',
            'interface_class': r'class\s+(\w+)\s*:\s*public\s+\w*I\w*',
            'enum_class': r'enum\s+class\s+(\w+)',
            'template_class': r'template\s*<[^>]*>\s*class\s+(\w+)',
            'function_template': r'template\s*<[^>]*>\s*(\w+)\s+\w+\s*\([^)]*\)',
        }
    
    def find_source_files(self) -> List[Path]:
        """查找所有源文件"""
        source_files = []
        for file_path in self.source_dir.rglob("*"):
            if file_path.suffix in ['.h', '.cpp', '.hpp']:
                source_files.append(file_path)
        return source_files
    
    def check_file_naming(self, file_path: Path) -> List[NamingIssue]:
        """检查单个文件的命名规范"""
        issues = []
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            
            for line_num, line in enumerate(lines, 1):
                line_issues = self.check_line_naming(line, line_num, file_path)
                issues.extend(line_issues)
        
        except Exception as e:
            issues.append(NamingIssue(
                file_path=file_path,
                line_number=0,
                issue_type="ERROR",
                message=f"无法读取文件: {str(e)}",
                suggestion="检查文件权限和编码"
            ))
        
        return issues
    
    def check_line_naming(self, line: str, line_num: int, file_path: Path) -> List[NamingIssue]:
        """检查单行的命名规范"""
        issues = []
        
        # 检查NObject子类
        nobject_match = re.search(self.patterns['nobject_class'], line)
        if nobject_match:
            class_name = nobject_match.group(1)
            if not class_name.startswith('N'):
                issues.append(NamingIssue(
                    file_path=file_path,
                    line_number=line_num,
                    issue_type="NAMING",
                    message=f"NObject子类 '{class_name}' 应该以 'N' 开头",
                    suggestion=f"将 '{class_name}' 重命名为 'N{class_name}'"
                ))
        
        # 检查普通类（不继承NObject）
        plain_match = re.search(self.patterns['plain_class'], line)
        if plain_match:
            class_name = plain_match.group(1)
            
            # 跳过模板类、枚举等
            if not any(keyword in line for keyword in ['template', 'enum', 'struct']):
                if class_name.startswith('N') and 'NCLASS' not in line:
                    issues.append(NamingIssue(
                        file_path=file_path,
                        line_number=line_num,
                        issue_type="NAMING",
                        message=f"普通类 '{class_name}' 以 'N' 开头但未继承NObject",
                        suggestion=f"将 '{class_name}' 重命名为 'C{class_name[1:]}' 或添加 NCLASS() 宏"
                    ))
                elif class_name.startswith('I') and 'virtual' not in line:
                    issues.append(NamingIssue(
                        file_path=file_path,
                        line_number=line_num,
                        issue_type="NAMING",
                        message=f"接口类 '{class_name}' 应该包含虚函数",
                        suggestion="添加纯虚函数或重命名为普通类"
                    ))
        
        # 检查枚举类
        enum_match = re.search(self.patterns['enum_class'], line)
        if enum_match:
            enum_name = enum_match.group(1)
            if not enum_name.startswith('E'):
                issues.append(NamingIssue(
                    file_path=file_path,
                    line_number=line_num,
                    issue_type="NAMING",
                    message=f"枚举类 '{enum_name}' 应该以 'E' 开头",
                    suggestion=f"将 '{enum_name}' 重命名为 'E{enum_name}'"
                ))
        
        # 检查模板类
        template_match = re.search(self.patterns['template_class'], line)
        if template_match:
            template_name = template_match.group(1)
            if not template_name.startswith('T'):
                issues.append(NamingIssue(
                    file_path=file_path,
                    line_number=line_num,
                    issue_type="NAMING",
                    message=f"模板类 '{template_name}' 应该以 'T' 开头",
                    suggestion=f"将 '{template_name}' 重命名为 'T{template_name}'"
                ))
        
        # 检查模板参数命名
        template_params = re.findall(r'template\s*<[^>]*>', line)
        for template_param in template_params:
            param_matches = re.findall(r'typename\s+(\w+)', template_param)
            for param in param_matches:
                if not self.is_valid_template_param_name(param):
                    issues.append(NamingIssue(
                        file_path=file_path,
                        line_number=line_num,
                        issue_type="NAMING",
                        message=f"模板参数 '{param}' 命名不规范",
                        suggestion=f"使用有意义的名称，如 TType, TKey, TValue, TFunction 等"
                    ))
        
        return issues
    
    def is_valid_template_param_name(self, param_name: str) -> bool:
        """检查模板参数名称是否有效"""
        valid_prefixes = ['T', 'U', 'V', 'W']  # 允许的模板参数前缀
        valid_names = [
            'TType', 'TKey', 'TValue', 'TFunction', 'TArgs', 'TIndex', 
            'TSize', 'TAllocator', 'TComparator', 'TPredicate'
        ]
        
        return (param_name in valid_names or 
                any(param_name.startswith(prefix) for prefix in valid_prefixes))
    
    def run_check(self) -> Dict:
        """运行完整的命名规范检查"""
        print("开始NLib命名规范检查...")
        print(f"源目录: {self.source_dir}")
        print("-" * 50)
        
        source_files = self.find_source_files()
        print(f"找到 {len(source_files)} 个源文件")
        
        total_issues = 0
        for file_path in source_files:
            file_issues = self.check_file_naming(file_path)
            self.issues.extend(file_issues)
            total_issues += len(file_issues)
        
        # 按类型分组问题
        issues_by_type = {}
        for issue in self.issues:
            if issue.issue_type not in issues_by_type:
                issues_by_type[issue.issue_type] = []
            issues_by_type[issue.issue_type].append(issue)
        
        # 输出结果
        print(f"\n检查完成，发现 {total_issues} 个问题:")
        
        for issue_type, issues in issues_by_type.items():
            print(f"\n{issue_type} 类型问题 ({len(issues)}):")
            for issue in issues:
                relative_path = issue.file_path.relative_to(self.source_dir)
                print(f"  {relative_path}:{issue.line_number} - {issue.message}")
                print(f"    建议: {issue.suggestion}")
        
        # 生成修复建议
        if total_issues > 0:
            self.generate_fix_suggestions()
        
        return {
            'total_files': len(source_files),
            'total_issues': total_issues,
            'issues_by_type': issues_by_type
        }
    
    def generate_fix_suggestions(self):
        """生成修复建议"""
        print(f"\n{'='*50}")
        print("修复建议:")
        print("1. 对于NObject子类，确保类名以 'N' 开头")
        print("2. 对于普通C++类，使用 'C' 前缀")
        print("3. 对于接口类，使用 'I' 前缀并包含纯虚函数")
        print("4. 对于枚举类，使用 'E' 前缀")
        print("5. 对于模板类，使用 'T' 前缀")
        print("6. 对于模板参数，使用有意义的名称如 TType, TKey, TValue 等")
        print("\n可以使用以下命令进行批量重命名:")
        print("  find . -name '*.h' -exec sed -i 's/class OldName/class NNewName/g' {} \\;")

def main():
    if len(sys.argv) < 2:
        print("用法: python check_naming.py <source_directory>")
        print("示例: python check_naming.py Sources/")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    
    if not os.path.exists(source_dir):
        print(f"错误: 目录不存在: {source_dir}")
        sys.exit(1)
    
    checker = NamingChecker(source_dir)
    result = checker.run_check()
    
    if result['total_issues'] > 0:
        print(f"\n发现 {result['total_issues']} 个命名规范问题，请修复后重新检查。")
        sys.exit(1)
    else:
        print("\n✓ 所有文件都符合命名规范！")

if __name__ == "__main__":
    main() 