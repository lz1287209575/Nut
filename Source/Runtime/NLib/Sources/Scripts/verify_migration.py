#!/usr/bin/env python3
"""
NLib 迁移验证脚本

验证反射系统和命名规范迁移的结果
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Dict

class MigrationVerifier:
    def __init__(self, source_dir: str):
        self.source_dir = Path(source_dir)
        self.verification_stats = {
            'total_files': 0,
            'nclass_files': 0,
            'generated_body_files': 0,
            'generate_includes': 0,
            'naming_compliant': 0,
            'issues': []
        }
    
    def find_source_files(self) -> List[Path]:
        """查找所有源文件"""
        source_files = []
        for file_path in self.source_dir.rglob("*"):
            if file_path.suffix in ['.h', '.cpp', '.hpp']:
                source_files.append(file_path)
        return source_files
    
    def verify_file(self, file_path: Path) -> Dict:
        """验证单个文件"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            stats = {
                'has_nclass': False,
                'has_generated_body': False,
                'has_generate_include': False,
                'naming_issues': []
            }
            
            # 检查NCLASS使用
            if 'NCLASS()' in content:
                stats['has_nclass'] = True
            
            # 检查GENERATED_BODY使用
            if 'GENERATED_BODY()' in content:
                stats['has_generated_body'] = True
            
            # 检查.generate.h包含
            if '.generate.h' in content:
                stats['has_generate_include'] = True
            
            # 检查命名规范
            naming_issues = self.check_naming_in_file(content, file_path)
            stats['naming_issues'] = naming_issues
            
            return stats
            
        except Exception as e:
            return {
                'has_nclass': False,
                'has_generated_body': False,
                'has_generate_include': False,
                'naming_issues': [f"Error reading file: {str(e)}"]
            }
    
    def check_naming_in_file(self, content: str, file_path: Path) -> List[str]:
        """检查文件中的命名规范"""
        issues = []
        
        # 检查NObject子类
        nobject_pattern = r'class\s+(\w+)\s*:\s*public\s+NObject'
        matches = re.findall(nobject_pattern, content)
        for class_name in matches:
            if not class_name.startswith('N'):
                issues.append(f"NObject子类 '{class_name}' 应该以 'N' 开头")
        
        # 检查普通类（不继承NObject且不是模板）
        plain_pattern = r'class\s+(\w+)\s*(?!.*:.*NObject)(?!.*template)'
        matches = re.findall(plain_pattern, content)
        for class_name in matches:
            # 跳过已知的接口类和特殊类
            if class_name.startswith('I') and 'virtual' in content:
                continue  # 这是接口类
            if class_name in ['Iterator', 'ConstIterator']:
                continue  # 这是迭代器类
            
            if class_name.startswith('N') and 'NCLASS' not in content:
                issues.append(f"普通类 '{class_name}' 以 'N' 开头但未继承NObject")
        
        return issues
    
    def run_verification(self) -> Dict:
        """运行完整的验证过程"""
        print("开始NLib迁移验证...")
        print(f"源目录: {self.source_dir}")
        print("-" * 50)
        
        source_files = self.find_source_files()
        self.verification_stats['total_files'] = len(source_files)
        
        print(f"找到 {len(source_files)} 个源文件")
        
        total_issues = 0
        for file_path in source_files:
            stats = self.verify_file(file_path)
            
            if stats['has_nclass']:
                self.verification_stats['nclass_files'] += 1
            
            if stats['has_generated_body']:
                self.verification_stats['generated_body_files'] += 1
            
            if stats['has_generate_include']:
                self.verification_stats['generate_includes'] += 1
            
            if not stats['naming_issues']:
                self.verification_stats['naming_compliant'] += 1
            else:
                total_issues += len(stats['naming_issues'])
                for issue in stats['naming_issues']:
                    self.verification_stats['issues'].append(f"{file_path.relative_to(self.source_dir)}: {issue}")
        
        # 输出验证结果
        print("\n" + "=" * 50)
        print("迁移验证结果:")
        print(f"  总文件数: {self.verification_stats['total_files']}")
        print(f"  使用NCLASS的文件: {self.verification_stats['nclass_files']}")
        print(f"  使用GENERATED_BODY的文件: {self.verification_stats['generated_body_files']}")
        print(f"  包含.generate.h的文件: {self.verification_stats['generate_includes']}")
        print(f"  命名规范符合的文件: {self.verification_stats['naming_compliant']}")
        print(f"  命名规范问题: {total_issues}")
        
        if self.verification_stats['issues']:
            print(f"\n发现的问题 ({len(self.verification_stats['issues'])}):")
            for issue in self.verification_stats['issues']:
                print(f"  ⚠ {issue}")
        
        # 计算成功率
        naming_success_rate = (self.verification_stats['naming_compliant'] / self.verification_stats['total_files']) * 100
        reflection_success_rate = (self.verification_stats['nclass_files'] / self.verification_stats['total_files']) * 100
        
        print(f"\n成功率:")
        print(f"  命名规范符合率: {naming_success_rate:.1f}%")
        print(f"  反射系统使用率: {reflection_success_rate:.1f}%")
        
        if total_issues == 0:
            print("\n🎉 恭喜！所有文件都符合命名规范！")
        elif total_issues <= 10:
            print(f"\n✅ 很好！只有 {total_issues} 个问题需要修复。")
        else:
            print(f"\n⚠️  还有 {total_issues} 个问题需要修复。")
        
        return self.verification_stats

def main():
    if len(sys.argv) < 2:
        print("用法: python verify_migration.py <source_directory>")
        print("示例: python verify_migration.py Sources/")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    
    if not os.path.exists(source_dir):
        print(f"错误: 目录不存在: {source_dir}")
        sys.exit(1)
    
    verifier = MigrationVerifier(source_dir)
    stats = verifier.run_verification()
    
    if len(stats['issues']) > 0:
        print(f"\n⚠️  验证完成，发现 {len(stats['issues'])} 个问题。")
        sys.exit(1)
    else:
        print(f"\n✅ 验证完成！所有文件都符合规范！")

if __name__ == "__main__":
    main() 