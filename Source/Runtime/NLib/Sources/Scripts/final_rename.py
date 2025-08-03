#!/usr/bin/env python3
"""
NLib 最终重命名脚本

处理剩余的命名规范问题
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Dict

class FinalRenamer:
    def __init__(self, source_dir: str):
        self.source_dir = Path(source_dir)
        self.rename_stats = {
            'files_processed': 0,
            'classes_renamed': 0,
            'errors': []
        }
        
        # 剩余需要重命名的类
        self.remaining_renames = {
            # 普通类 -> C前缀
            'NScriptEngineManager': 'CScriptEngineManager',
            'NExampleNObject': 'CExampleNObject',
            'NConfigManager': 'CConfigManager',
            'NJsonConfigLoader': 'CJsonConfigLoader',
            'NIniConfigLoader': 'CIniConfigLoader',
            'NConfigWatcher': 'CConfigWatcher',
            'NDateTime': 'CDateTime',
            'NStopwatch': 'CStopwatch',
            'NBinaryArchive': 'CBinaryArchive',
            'NJsonArchive': 'CJsonArchive',
            'NMemoryArchive': 'CMemoryArchive',
            'NVersionedSerializer': 'CVersionedSerializer',
            'NDataResource': 'CDataResource',
            'NTextResource': 'CTextResource',
            'NConfigResource': 'CConfigResource',
            'NUdpReactor': 'CUdpReactor',
            'NIOEvent': 'CIOEvent',
            'NIOCPEventLoop': 'CIOCPEventLoop',
            'NEventLoopManager': 'CEventLoopManager',
            'NCoroutineAwaiter': 'CCoroutineAwaiter',
            'NTimeAwaiter': 'CTimeAwaiter',
            'NCoroutineChannel': 'CCoroutineChannel',
            'NCoroutineGenerator': 'CCoroutineGenerator',
            'NFuture': 'CFuture',
            'NCancellationToken': 'CCancellationToken',
            'NParallelExecutor': 'CParallelExecutor',
            'NRunnable': 'CRunnable',
            'NFunctionRunnable': 'CFunctionRunnable',
            'NPeriodicRunnable': 'CPeriodicRunnable',
            'NRunnablePool': 'CRunnablePool',
            'NFileInfo': 'CFileInfo',
            'NDirectoryInfo': 'CDirectoryInfo',
            'NStream': 'CStream',
            'NFileStream': 'CFileStream',
            'NPath': 'CPath',
            'NFile': 'CFile',
            'NDirectory': 'CDirectory',
            'NDualQuaternion': 'CDualQuaternion',
            'NVector2': 'CVector2',
            'NVector3': 'CVector3',
            'NVector4': 'CVector4',
            'NDelegateBase': 'CDelegateBase',
            'NEvent': 'CEvent',
            'NEventFilter': 'CEventFilter',
            'NEventDispatcher': 'CEventDispatcher',
            'NScopedEventHandler': 'CScopedEventHandler',
            'NEventBus': 'CEventBus',
            'NMutex': 'CMutex',
            'NReadWriteLock': 'CReadWriteLock',
            'NConditionVariable': 'CConditionVariable',
            'NTask': 'CTask',
            'NThreadPool': 'CThreadPool',
            'NTimer': 'CTimer',
            'NSemaphore': 'CSemaphore',
        }
        
        # 模板类 -> T前缀
        self.template_renames = {
            'CWeakPtr': 'TWeakPtr',
            'CSharedRef': 'TSharedRef',
            'CWeakRef': 'TWeakRef',
            'CSharedPtr': 'TSharedPtr',
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
            for old_name, new_name in self.remaining_renames.items():
                if old_name in content:
                    pattern = r'\b' + re.escape(old_name) + r'\b'
                    if re.search(pattern, content):
                        content = re.sub(pattern, new_name, content)
                        changes_made = True
                        self.rename_stats['classes_renamed'] += 1
            
            # 2. 重命名模板类
            for old_name, new_name in self.template_renames.items():
                if old_name in content:
                    pattern = r'\b' + re.escape(old_name) + r'\b'
                    if re.search(pattern, content):
                        content = re.sub(pattern, new_name, content)
                        changes_made = True
                        self.rename_stats['classes_renamed'] += 1
            
            # 3. 修复接口类（添加虚函数）
            # 查找没有虚函数的接口类
            interface_classes = ['IAllocator', 'ISerializable', 'IEventHandler', 'IScriptEngine']
            for interface_name in interface_classes:
                if interface_name in content:
                    # 检查是否已经有虚函数
                    if 'virtual' not in content or '= 0' not in content:
                        # 在类定义中添加纯虚函数
                        class_pattern = rf'class\s+{re.escape(interface_name)}\s*[^{{]*{{'
                        if re.search(class_pattern, content):
                            # 添加一个默认的纯虚析构函数
                            replacement = f'class {interface_name} {{\npublic:\n    virtual ~{interface_name}() = default;'
                            content = re.sub(class_pattern, replacement, content)
                            changes_made = True
            
            # 4. 修复Iterator接口
            iterator_pattern = r'class\s+Iterator\s*[^}]*{'
            if re.search(iterator_pattern, content):
                # 添加虚函数到Iterator类
                replacement = 'class Iterator {\npublic:\n    virtual ~Iterator() = default;\n    virtual void Next() = 0;\n    virtual bool HasNext() const = 0;'
                content = re.sub(iterator_pattern, replacement, content)
                changes_made = True
            
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
        print("开始NLib最终重命名...")
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
        
        if self.rename_stats['errors']:
            print(f"\n错误 ({len(self.rename_stats['errors'])}):")
            for error in self.rename_stats['errors']:
                print(f"  ✗ {error}")
        
        return self.rename_stats

def main():
    if len(sys.argv) < 2:
        print("用法: python final_rename.py <source_directory>")
        print("示例: python final_rename.py Sources/")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    
    if not os.path.exists(source_dir):
        print(f"错误: 目录不存在: {source_dir}")
        sys.exit(1)
    
    print("🔧 执行最终重命名，处理剩余的命名规范问题...")
    
    renamer = FinalRenamer(source_dir)
    stats = renamer.run_renaming()
    
    if stats['errors']:
        print(f"\n⚠️  重命名过程中遇到 {len(stats['errors'])} 个错误，请检查。")
        sys.exit(1)
    else:
        print(f"\n✅ 最终重命名完成！共处理 {stats['files_processed']} 个文件。")

if __name__ == "__main__":
    main() 