#!/usr/bin/env python3
"""
NLib 关键错误修复脚本

修复迁移过程中的关键错误：
1. 恢复NObject作为基类
2. 修复重复的类声明
3. 纠正错误的继承关系
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Dict

class CriticalErrorFixer:
    def __init__(self, source_dir: str):
        self.source_dir = Path(source_dir)
        self.fix_stats = {
            'files_processed': 0,
            'nobject_restored': 0,
            'duplicates_fixed': 0,
            'inheritance_fixed': 0,
            'errors': []
        }
        
        # 需要恢复为NObject的类（这些应该继承NObject）
        self.nobject_classes = {
            'CCancellationToken': 'NCancellationToken',
            'CAsyncTask': 'NAsyncTask',
            'CAsyncTaskBase': 'NAsyncTaskBase',
            'CAsyncTaskScheduler': 'NAsyncTaskScheduler',
            'CParallelExecutor': 'NParallelExecutor',
            'CAsyncResult': 'NAsyncResult',
            'CConfigManager': 'NConfigManager',
            'CJsonConfigLoader': 'NJsonConfigLoader',
            'CIniConfigLoader': 'NIniConfigLoader',
            'CConfigWatcher': 'NConfigWatcher',
            'CDateTime': 'NDateTime',
            'CStopwatch': 'NStopwatch',
            'CBinaryArchive': 'NBinaryArchive',
            'CJsonArchive': 'NJsonArchive',
            'CMemoryArchive': 'NMemoryArchive',
            'CVersionedSerializer': 'NVersionedSerializer',
            'CDataResource': 'NDataResource',
            'CTextResource': 'NTextResource',
            'CConfigResource': 'NConfigResource',
            'CUdpReactor': 'NUdpReactor',
            'CIOEvent': 'NIOEvent',
            'CIOCPEventLoop': 'NIOCPEventLoop',
            'CEventLoopManager': 'NEventLoopManager',
            'CCoroutineAwaiter': 'NCoroutineAwaiter',
            'CTimeAwaiter': 'NTimeAwaiter',
            'CCoroutineChannel': 'NCoroutineChannel',
            'CCoroutineGenerator': 'NCoroutineGenerator',
            'CFuture': 'NFuture',
            'CRunnable': 'NRunnable',
            'CFunctionRunnable': 'NFunctionRunnable',
            'CPeriodicRunnable': 'NPeriodicRunnable',
            'CRunnablePool': 'NRunnablePool',
            'CFileInfo': 'NFileInfo',
            'CDirectoryInfo': 'NDirectoryInfo',
            'CStream': 'NStream',
            'CFileStream': 'NFileStream',
            'CPath': 'NPath',
            'CFile': 'NFile',
            'CDirectory': 'NDirectory',
            'CDualQuaternion': 'NDualQuaternion',
            'CVector2': 'NVector2',
            'CVector3': 'NVector3',
            'CVector4': 'NVector4',
            'CDelegateBase': 'NDelegateBase',
            'CEvent': 'NEvent',
            'CEventFilter': 'NEventFilter',
            'CEventDispatcher': 'NEventDispatcher',
            'CScopedEventHandler': 'NScopedEventHandler',
            'CEventBus': 'NEventBus',
            'CMutex': 'NMutex',
            'CReadWriteLock': 'NReadWriteLock',
            'CConditionVariable': 'NConditionVariable',
            'CTask': 'NTask',
            'CThreadPool': 'NThreadPool',
            'CTimer': 'NTimer',
            'CSemaphore': 'NSemaphore',
            'CScriptEngineManager': 'NScriptEngineManager',
            'CExampleNObject': 'NExampleNObject',
        }
    
    def find_source_files(self) -> List[Path]:
        """查找所有源文件"""
        source_files = []
        for file_path in self.source_dir.rglob("*"):
            if file_path.suffix in ['.h', '.cpp', '.hpp']:
                source_files.append(file_path)
        return source_files
    
    def fix_file(self, file_path: Path) -> bool:
        """修复单个文件的关键错误"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            original_content = content
            changes_made = False
            
            # 1. 修复重复的类声明
            # 查找类似 "class CCancellationToken : public CObject\n{\n    NCLASS()\nclass CCancellationToken : public CObject" 的模式
            duplicate_pattern = r'class\s+(\w+)\s*:\s*public\s+CObject\s*\{\s*NCLASS\(\)\s*class\s+\1\s*:\s*public\s+CObject'
            matches = re.findall(duplicate_pattern, content, re.DOTALL)
            
            for class_name in matches:
                if class_name in self.nobject_classes:
                    # 修复重复声明，恢复为NObject继承
                    old_pattern = f'class {class_name} : public CObject\\s*{{\\s*NCLASS\\(\\)\\s*class {class_name} : public CObject'
                    new_pattern = f'class {self.nobject_classes[class_name]} : public NObject\\n{{\\n    NCLASS()\\nclass {self.nobject_classes[class_name]} : public NObject'
                    
                    content = re.sub(old_pattern, new_pattern, content, flags=re.DOTALL)
                    changes_made = True
                    self.fix_stats['duplicates_fixed'] += 1
                    self.fix_stats['nobject_restored'] += 1
            
            # 2. 修复继承关系：CObject -> NObject（对于应该继承NObject的类）
            for old_name, new_name in self.nobject_classes.items():
                if old_name in content:
                    # 修复继承关系
                    inheritance_pattern = f'class\\s+{re.escape(old_name)}\\s*:\\s*public\\s+CObject'
                    if re.search(inheritance_pattern, content):
                        content = re.sub(inheritance_pattern, f'class {new_name} : public NObject', content)
                        changes_made = True
                        self.fix_stats['inheritance_fixed'] += 1
                    
                    # 修复类名引用
                    class_ref_pattern = r'\b' + re.escape(old_name) + r'\b'
                    content = re.sub(class_ref_pattern, new_name, content)
                    changes_made = True
            
            # 3. 修复模板参数错误
            # 修复 T -> TType 的错误
            content = re.sub(r'\bT\b(?!ype)', 'TType', content)
            
            # 4. 修复CObject -> NObject的基类引用
            # 但只修复那些应该继承NObject的类
            for old_name, new_name in self.nobject_classes.items():
                if old_name in content:
                    # 修复基类引用
                    base_ref_pattern = f'\\b{re.escape(old_name)}\\b'
                    content = re.sub(base_ref_pattern, new_name, content)
            
            # 如果内容有变化，写回文件
            if changes_made:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(content)
                self.fix_stats['files_processed'] += 1
                return True
            
            return False
            
        except Exception as e:
            self.fix_stats['errors'].append(f"Error processing {file_path}: {str(e)}")
            return False
    
    def run_fix(self) -> Dict:
        """运行完整的修复过程"""
        print("开始修复NLib关键错误...")
        print(f"源目录: {self.source_dir}")
        print("-" * 50)
        
        source_files = self.find_source_files()
        print(f"找到 {len(source_files)} 个源文件")
        
        fixed_files = []
        for file_path in source_files:
            if self.fix_file(file_path):
                fixed_files.append(file_path)
                print(f"✓ 修复文件: {file_path.relative_to(self.source_dir)}")
        
        # 输出统计信息
        print("\n" + "=" * 50)
        print("修复统计:")
        print(f"  处理的文件: {self.fix_stats['files_processed']}")
        print(f"  恢复的NObject类: {self.fix_stats['nobject_restored']}")
        print(f"  修复的重复声明: {self.fix_stats['duplicates_fixed']}")
        print(f"  修复的继承关系: {self.fix_stats['inheritance_fixed']}")
        
        if self.fix_stats['errors']:
            print(f"\n错误 ({len(self.fix_stats['errors'])}):")
            for error in self.fix_stats['errors']:
                print(f"  ✗ {error}")
        
        return self.fix_stats

def main():
    if len(sys.argv) < 2:
        print("用法: python fix_critical_errors.py <source_directory>")
        print("示例: python fix_critical_errors.py Sources/")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    
    if not os.path.exists(source_dir):
        print(f"错误: 目录不存在: {source_dir}")
        sys.exit(1)
    
    print("🔧 修复关键错误：恢复NObject基类和修复重复声明...")
    
    fixer = CriticalErrorFixer(source_dir)
    stats = fixer.run_fix()
    
    if stats['errors']:
        print(f"\n⚠️  修复过程中遇到 {len(stats['errors'])} 个错误，请检查。")
        sys.exit(1)
    else:
        print(f"\n✅ 关键错误修复完成！共处理 {stats['files_processed']} 个文件。")

if __name__ == "__main__":
    main() 