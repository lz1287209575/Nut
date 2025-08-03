#!/usr/bin/env python3
"""
NLib 反射系统迁移脚本

此脚本帮助将 DECLARE_NOBJECT_CLASS 系统迁移到 NCLASS + GENERATED_BODY 系统
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Tuple, Dict

class ReflectionMigrator:
    def __init__(self, source_dir: str):
        self.source_dir = Path(source_dir)
        self.migration_stats = {
            'files_processed': 0,
            'classes_migrated': 0,
            'includes_added': 0,
            'errors': []
        }
    
    def find_header_files(self) -> List[Path]:
        """查找所有需要处理的头文件"""
        header_files = []
        for file_path in self.source_dir.rglob("*.h"):
            if file_path.name.endswith('.h') and not file_path.name.endswith('.generate.h'):
                header_files.append(file_path)
        return header_files
    
    def find_cpp_files(self) -> List[Path]:
        """查找所有需要处理的cpp文件"""
        cpp_files = []
        for file_path in self.source_dir.rglob("*.cpp"):
            cpp_files.append(file_path)
        return cpp_files
    
    def parse_declare_nobject_class(self, content: str) -> List[Tuple[str, str]]:
        """解析DECLARE_NOBJECT_CLASS声明"""
        pattern = r'DECLARE_NOBJECT_CLASS\((\w+),\s*(\w+)\)'
        matches = re.findall(pattern, content)
        return matches
    
    def migrate_header_file(self, file_path: Path) -> bool:
        """迁移单个头文件"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            original_content = content
            
            # 查找DECLARE_NOBJECT_CLASS声明
            class_declarations = self.parse_declare_nobject_class(content)
            
            if not class_declarations:
                return False
            
            # 迁移每个类声明
            for class_name, super_class in class_declarations:
                # 替换DECLARE_NOBJECT_CLASS为NCLASS + GENERATED_BODY
                old_pattern = f'DECLARE_NOBJECT_CLASS\\({class_name},\\s*{super_class}\\)'
                new_content = f'NCLASS()\\nclass {class_name} : public {super_class}\\n{{\\n    GENERATED_BODY()'
                
                content = re.sub(old_pattern, new_content, content)
                self.migration_stats['classes_migrated'] += 1
            
            # 如果内容有变化，写回文件
            if content != original_content:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(content)
                self.migration_stats['files_processed'] += 1
                return True
            
            return False
            
        except Exception as e:
            self.migration_stats['errors'].append(f"Error processing {file_path}: {str(e)}")
            return False
    
    def add_generate_includes(self, cpp_file: Path) -> bool:
        """在cpp文件中添加.generate.h包含"""
        try:
            with open(cpp_file, 'r', encoding='utf-8') as f:
                content = f.read()
            
            original_content = content
            
            # 查找对应的头文件
            header_file = cpp_file.with_suffix('.h')
            if not header_file.exists():
                return False
            
            # 检查头文件是否包含NCLASS
            with open(header_file, 'r', encoding='utf-8') as f:
                header_content = f.read()
            
            if 'NCLASS()' not in header_content:
                return False
            
            # 检查是否已经包含.generate.h文件
            generate_include = f'#include "{header_file.name.replace(".h", ".generate.h")}"'
            if generate_include in content:
                return False
            
            # 在头文件包含后添加.generate.h包含
            header_include = f'#include "{header_file.name}"'
            if header_include in content:
                # 找到头文件包含的位置
                lines = content.split('\n')
                for i, line in enumerate(lines):
                    if header_include in line:
                        # 在下一行插入.generate.h包含
                        lines.insert(i + 1, generate_include)
                        break
                
                content = '\n'.join(lines)
                
                if content != original_content:
                    with open(cpp_file, 'w', encoding='utf-8') as f:
                        f.write(content)
                    self.migration_stats['includes_added'] += 1
                    return True
            
            return False
            
        except Exception as e:
            self.migration_stats['errors'].append(f"Error processing {cpp_file}: {str(e)}")
            return False
    
    def check_naming_convention(self, file_path: Path) -> List[str]:
        """检查命名规范"""
        issues = []
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # 查找类声明
            class_pattern = r'class\s+(\w+)\s*:\s*public\s+NObject'
            matches = re.findall(class_pattern, content)
            
            for class_name in matches:
                if not class_name.startswith('N'):
                    issues.append(f"Class {class_name} should start with 'N' (NObject-derived class)")
            
            # 查找普通类声明（不继承NObject）
            plain_class_pattern = r'class\s+(\w+)\s*(?!.*:.*NObject)'
            plain_matches = re.findall(plain_class_pattern, content)
            
            for class_name in plain_matches:
                if class_name.startswith('N') and 'NCLASS' not in content:
                    issues.append(f"Class {class_name} starts with 'N' but doesn't inherit from NObject")
        
        except Exception as e:
            issues.append(f"Error checking naming convention: {str(e)}")
        
        return issues
    
    def run_migration(self, dry_run: bool = False) -> Dict:
        """运行完整的迁移过程"""
        print("开始NLib反射系统迁移...")
        print(f"源目录: {self.source_dir}")
        print(f"模式: {'干运行' if dry_run else '实际迁移'}")
        print("-" * 50)
        
        # 迁移头文件
        header_files = self.find_header_files()
        print(f"找到 {len(header_files)} 个头文件")
        
        migrated_headers = []
        for header_file in header_files:
            if self.migrate_header_file(header_file):
                migrated_headers.append(header_file)
                print(f"✓ 迁移头文件: {header_file.relative_to(self.source_dir)}")
        
        # 添加.generate.h包含
        cpp_files = self.find_cpp_files()
        print(f"找到 {len(cpp_files)} 个cpp文件")
        
        migrated_cpps = []
        for cpp_file in cpp_files:
            if self.add_generate_includes(cpp_file):
                migrated_cpps.append(cpp_file)
                print(f"✓ 添加包含: {cpp_file.relative_to(self.source_dir)}")
        
        # 检查命名规范
        print("\n检查命名规范...")
        naming_issues = []
        for header_file in header_files:
            issues = self.check_naming_convention(header_file)
            if issues:
                naming_issues.extend([f"{header_file.relative_to(self.source_dir)}: {issue}" for issue in issues])
        
        # 输出统计信息
        print("\n" + "=" * 50)
        print("迁移统计:")
        print(f"  处理的文件: {self.migration_stats['files_processed']}")
        print(f"  迁移的类: {self.migration_stats['classes_migrated']}")
        print(f"  添加的包含: {self.migration_stats['includes_added']}")
        
        if naming_issues:
            print(f"\n命名规范问题 ({len(naming_issues)}):")
            for issue in naming_issues:
                print(f"  ⚠ {issue}")
        
        if self.migration_stats['errors']:
            print(f"\n错误 ({len(self.migration_stats['errors'])}):")
            for error in self.migration_stats['errors']:
                print(f"  ✗ {error}")
        
        return self.migration_stats

def main():
    if len(sys.argv) < 2:
        print("用法: python migrate_reflection.py <source_directory> [--dry-run]")
        print("示例: python migrate_reflection.py Sources/ --dry-run")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    dry_run = '--dry-run' in sys.argv
    
    if not os.path.exists(source_dir):
        print(f"错误: 目录不存在: {source_dir}")
        sys.exit(1)
    
    migrator = ReflectionMigrator(source_dir)
    stats = migrator.run_migration(dry_run)
    
    if stats['errors']:
        sys.exit(1)

if __name__ == "__main__":
    main() 