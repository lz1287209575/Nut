#!/usr/bin/env python3
"""
NLib 自动重命名脚本

激进地批量重命名所有不符合命名规范的类
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple

class AutoRenamer:
    def __init__(self, source_dir: str):
        self.source_dir = Path(source_dir)
        self.rename_stats = {
            'files_processed': 0,
            'classes_renamed': 0,
            'template_params_renamed': 0,
            'errors': []
        }
        
        # 重命名映射
        self.class_renames = {
            # 普通类 -> C前缀
            'NLogger': 'CLogger',
            'NConfigParser': 'CConfigParser',
            'NFileWatcher': 'CFileWatcher',
            'NPerformanceCounter': 'CPerformanceCounter',
            'NObjectReflection': 'CObjectReflection',
            'NClassRegistrar': 'CClassRegistrar',
            'NScriptValue': 'CScriptValue',
            'NScriptContext': 'CScriptContext',
            'NScriptMetaRegistrar': 'CScriptMetaRegistrar',
            'NPlayer': 'CPlayer',
            'NBPExecutionContext': 'CBPExecutionContext',
            'NLibModule': 'CLibModule',
            'NGarbageCollector': 'CGarbageCollector',
            'NObjectRegistry': 'CObjectRegistry',
            'NUniquePtr': 'CUniquePtr',
            'NSharedPtr': 'CSharedPtr',
            'NWeakPtr': 'CWeakPtr',
            'NSharedRef': 'CSharedRef',
            'NWeakRef': 'CWeakRef',
            'NPointerControlBlock': 'CPointerControlBlock',
            'NInPlaceControlBlock': 'CInPlaceControlBlock',
            'NObjectControlBlock': 'CObjectControlBlock',
            'NSharedFromThis': 'CSharedFromThis',
            'NAllocator': 'CAllocator',
            'NMemoryManager': 'CMemoryManager',
            'NThread': 'CThread',
            'NPromise': 'CPromise',
            'NLazyFuture': 'CLazyFuture',
            'NAsyncResult': 'CAsyncResult',
            'NAsyncTask': 'CAsyncTask',
            'NQuaternion': 'CQuaternion',
            'NMatrix3': 'CMatrix3',
            'NMatrix4': 'CMatrix4',
            'NDelegate': 'CDelegate',
            'NMulticastDelegate': 'CMulticastDelegate',
            'NEventHandler': 'CEventHandler',
            'NAsyncTaskScheduler': 'CAsyncTaskScheduler',
            'NLockGuard': 'CLockGuard',
            'NReadLockGuard': 'CReadLockGuard',
            'NWriteLockGuard': 'CWriteLockGuard',
            'NThreadLocal': 'CThreadLocal',
            'NAtomic': 'CAtomic',
            'NCoroutineScheduler': 'CCoroutineScheduler',
            'NJsonValue': 'CJsonValue',
            'NArchive': 'CArchive',
            'NSerializationContext': 'CSerializationContext',
            'NResourceFactory': 'CResourceFactory',
            'NConfigValue': 'CConfigValue',
            'NResourceHandle': 'CResourceHandle',
            'NIteratorBase': 'CIteratorBase',
            'NArray': 'CArray',
            'NSet': 'CSet',
            'NContainer': 'CContainer',
            'NString': 'CString',
            'NHashMap': 'CHashMap',
            'NObject': 'CObject',
        }
        
        # 模板类 -> T前缀
        self.template_class_renames = {
            'NSharedPtr': 'TSharedPtr',
            'NWeakPtr': 'TWeakPtr',
            'NSharedRef': 'TSharedRef',
            'NWeakRef': 'TWeakRef',
            'NArray': 'TArray',
            'NHashMap': 'THashMap',
            'NSet': 'TSet',
            'NUniquePtr': 'TUniquePtr',
        }
        
        # 枚举类 -> E前缀
        self.enum_renames = {
            'LogLevel': 'ELogLevel',
        }
        
        # 模板参数重命名
        self.template_param_renames = {
            'K': 'TKey',
            'V': 'TValue',
            'T': 'TType',
            'Args': 'TArgs',
            'E': 'TType',
            'D': 'TDeleter',
            'D1': 'TDeleter1',
            'D2': 'TDeleter2',
            'PtrType': 'TPtrType',
            'ReturnType': 'TReturnType',
            'ObjectType': 'TObjectType',
            'LambdaType': 'TLambdaType',
            'ContainerType': 'TContainerType',
            'InputIterator': 'TInputIterator',
            'Compare': 'TCompare',
            'Predicate': 'TPredicate',
        }
    
    def find_source_files(self) -> List[Path]:
        """查找所有源文件"""
        source_files = []
        for file_path in self.source_dir.rglob("*"):
            if file_path.suffix in ['.h', '.cpp', '.hpp']:
                source_files.append(file_path)
        return source_files
    
    def rename_in_file(self, file_path: Path) -> bool:
        """在单个文件中执行重命名"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            original_content = content
            changes_made = False
            
            # 1. 重命名普通类
            for old_name, new_name in self.class_renames.items():
                if old_name in content:
                    # 使用单词边界确保精确匹配
                    pattern = r'\b' + re.escape(old_name) + r'\b'
                    if re.search(pattern, content):
                        content = re.sub(pattern, new_name, content)
                        changes_made = True
                        self.rename_stats['classes_renamed'] += 1
            
            # 2. 重命名模板类（在template声明中）
            for old_name, new_name in self.template_class_renames.items():
                if old_name in content:
                    # 查找template<typename...> class OldName模式
                    pattern = r'template\s*<[^>]*>\s*class\s+' + re.escape(old_name) + r'\b'
                    if re.search(pattern, content):
                        content = re.sub(pattern, lambda m: m.group(0).replace(old_name, new_name), content)
                        changes_made = True
                        self.rename_stats['classes_renamed'] += 1
            
            # 3. 重命名枚举类
            for old_name, new_name in self.enum_renames.items():
                if old_name in content:
                    pattern = r'enum\s+class\s+' + re.escape(old_name) + r'\b'
                    if re.search(pattern, content):
                        content = re.sub(pattern, lambda m: m.group(0).replace(old_name, new_name), content)
                        changes_made = True
                        self.rename_stats['classes_renamed'] += 1
            
            # 4. 重命名模板参数
            for old_name, new_name in self.template_param_renames.items():
                if old_name in content:
                    # 查找typename OldName模式
                    pattern = r'typename\s+' + re.escape(old_name) + r'\b'
                    if re.search(pattern, content):
                        content = re.sub(pattern, lambda m: m.group(0).replace(old_name, new_name), content)
                        changes_made = True
                        self.rename_stats['template_params_renamed'] += 1
            
            # 5. 重命名接口类（添加I前缀）
            interface_pattern = r'class\s+([A-Z][a-zA-Z0-9]*)\s*:\s*public\s+[A-Z][a-zA-Z0-9]*\s*\{[^}]*virtual[^}]*= 0[^}]*\}'
            interface_matches = re.findall(interface_pattern, content, re.DOTALL)
            for interface_name in interface_matches:
                if not interface_name.startswith('I') and not interface_name.startswith('N'):
                    new_name = 'I' + interface_name
                    pattern = r'\b' + re.escape(interface_name) + r'\b'
                    if re.search(pattern, content):
                        content = re.sub(pattern, new_name, content)
                        changes_made = True
                        self.rename_stats['classes_renamed'] += 1
            
            # 如果内容有变化，写回文件
            if changes_made:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(content)
                self.rename_stats['files_processed'] += 1
                return True
            
            return False
            
        except Exception as e:
            self.rename_stats['errors'].append(f"Error processing {file_path}: {str(e)}")
            return False
    
    def run_renaming(self) -> Dict:
        """运行完整的重命名过程"""
        print("开始NLib自动重命名...")
        print(f"源目录: {self.source_dir}")
        print("-" * 50)
        
        source_files = self.find_source_files()
        print(f"找到 {len(source_files)} 个源文件")
        
        renamed_files = []
        for file_path in source_files:
            if self.rename_in_file(file_path):
                renamed_files.append(file_path)
                print(f"✓ 重命名文件: {file_path.relative_to(self.source_dir)}")
        
        # 输出统计信息
        print("\n" + "=" * 50)
        print("重命名统计:")
        print(f"  处理的文件: {self.rename_stats['files_processed']}")
        print(f"  重命名的类: {self.rename_stats['classes_renamed']}")
        print(f"  重命名的模板参数: {self.rename_stats['template_params_renamed']}")
        
        if self.rename_stats['errors']:
            print(f"\n错误 ({len(self.rename_stats['errors'])}):")
            for error in self.rename_stats['errors']:
                print(f"  ✗ {error}")
        
        return self.rename_stats

def main():
    if len(sys.argv) < 2:
        print("用法: python auto_rename.py <source_directory>")
        print("示例: python auto_rename.py Sources/")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    
    if not os.path.exists(source_dir):
        print(f"错误: 目录不存在: {source_dir}")
        sys.exit(1)
    
    print("⚠️  警告: 这是一个激进的自动重命名脚本！")
    print("它将批量重命名所有不符合命名规范的类。")
    print("建议先备份代码或使用版本控制。")
    
    confirm = input("确认继续吗？(y/N): ")
    if confirm.lower() != 'y':
        print("操作已取消。")
        sys.exit(0)
    
    renamer = AutoRenamer(source_dir)
    stats = renamer.run_renaming()
    
    if stats['errors']:
        print(f"\n⚠️  重命名过程中遇到 {len(stats['errors'])} 个错误，请检查。")
        sys.exit(1)
    else:
        print(f"\n✅ 自动重命名完成！共处理 {stats['files_processed']} 个文件。")

if __name__ == "__main__":
    main() 