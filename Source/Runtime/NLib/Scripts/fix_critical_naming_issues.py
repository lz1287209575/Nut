#!/usr/bin/env python3
"""
修复关键的命名问题脚本
主要修复：
1. 将CObject恢复为NObject（核心基类不应该被重命名）
2. 修复重复的类定义
3. 修复相关的引用和依赖
"""

import os
import re
import glob
from pathlib import Path

def fix_critical_naming_issues():
    """修复关键的命名问题"""
    
    # 获取所有源文件
    source_files = []
    for ext in ['*.h', '*.cpp', '*.hpp']:
        source_files.extend(glob.glob(f'Sources/**/{ext}', recursive=True))
    
    print(f"找到 {len(source_files)} 个源文件")
    
    # 修复统计
    fixed_files = 0
    total_changes = 0
    
    for file_path in source_files:
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            original_content = content
            changes_made = 0
        except Exception as e:
            print(f"读取文件失败: {file_path} - {e}")
            continue
            
            # 1. 修复CObject -> NObject（核心基类）
            # 但要注意不要修改NObjectSmartPointers中的CObjectControlBlock
            content = re.sub(r'\bCObject\b(?!ControlBlock)', 'NObject', content)
            
            # 2. 修复重复的类定义（在NAsyncTask.h中）
            if 'NAsyncTask.h' in file_path:
                # 移除重复的类定义
                # 找到第一个NAsyncTask类定义后的重复定义
                pattern = r'(template<>\s*class\s+LIBNLIB_API\s+NAsyncTask<void>\s*:\s*public\s+NAsyncTaskBase\s*\{[^}]*\})\s*class\s+NAsyncTask\s*:\s*public\s+NAsyncTaskBase'
                replacement = r'\1'
                content = re.sub(pattern, replacement, content, flags=re.DOTALL)
                
                # 修复NParallelExecutor的重复定义
                pattern = r'(class\s+LIBNLIB_API\s+NParallelExecutor\s*:\s*public\s+NObject\s*\{[^}]*\})\s*class\s+NParallelExecutor\s*:\s*public\s+NObject'
                replacement = r'\1'
                content = re.sub(pattern, replacement, content, flags=re.DOTALL)
                
                # 修复NCancellationToken的重复定义
                pattern = r'(class\s+LIBNLIB_API\s+NCancellationToken\s*:\s*public\s+NObject\s*\{[^}]*\})\s*class\s+NCancellationToken\s*:\s*public\s+NObject'
                replacement = r'\1'
                content = re.sub(pattern, replacement, content, flags=re.DOTALL)
            
            # 3. 修复相关的类型引用
            content = re.sub(r'\bTSharedPtr<CObject>\b', 'TSharedPtr<NObject>', content)
            content = re.sub(r'\bCObject\*\b', 'NObject*', content)
            content = re.sub(r'\bconst\s+CObject\*\b', 'const NObject*', content)
            
            # 4. 修复函数参数和返回类型
            content = re.sub(r'\(\s*CObject\s*\*', '(NObject*', content)
            content = re.sub(r'\(\s*const\s+CObject\s*\*', '(const NObject*', content)
            content = re.sub(r'->\s*CObject\s*\*', '-> NObject*', content)
            
            # 5. 修复模板参数
            content = re.sub(r'std::is_base_of_v<CObject,', 'std::is_base_of_v<NObject,', content)
            content = re.sub(r'std::is_base_of_v<NObject,', 'std::is_base_of_v<NObject,', content)
            
            # 6. 修复注释中的引用
            content = re.sub(r'@brief\s+CObject', '@brief NObject', content)
            content = re.sub(r'继承自\s+CObject', '继承自NObject', content)
            content = re.sub(r'CObject\s+的', 'NObject 的', content)
            
            # 7. 修复字符串中的引用
            content = re.sub(r'"CObject"', '"NObject"', content)
            content = re.sub(r"'CObject'", "'NObject'", content)
            
            # 检查是否有变化
            if content != original_content:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(content)
                fixed_files += 1
                changes = len(re.findall(r'CObject', original_content)) - len(re.findall(r'CObject', content))
                total_changes += changes
                print(f"修复文件: {file_path} (修改了 {changes} 处)")
    
    print(f"\n修复完成！")
    print(f"修复了 {fixed_files} 个文件")
    print(f"总共修改了 {total_changes} 处")
    
    # 验证修复结果
    print("\n验证修复结果...")
    remaining_cobject = 0
    for file_path in source_files:
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            count = len(re.findall(r'\bCObject\b(?!ControlBlock)', content))
            if count > 0:
                print(f"警告: {file_path} 仍有 {count} 个CObject引用")
                remaining_cobject += count
        except Exception as e:
            print(f"读取文件失败: {file_path} - {e}")
    
    if remaining_cobject == 0:
        print("✅ 所有CObject引用已成功修复为NObject")
    else:
        print(f"⚠️  仍有 {remaining_cobject} 个CObject引用需要手动检查")

def fix_duplicate_class_definitions():
    """专门修复重复的类定义"""
    
    # 修复NAsyncTask.h中的重复定义
    async_task_file = 'Sources/Async/NAsyncTask.h'
    if os.path.exists(async_task_file):
        with open(async_task_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 修复void特化版本的重复定义
        # 找到第一个完整的定义并移除重复的部分
        lines = content.split('\n')
        fixed_lines = []
        in_void_specialization = False
        void_specialization_complete = False
        
        i = 0
        while i < len(lines):
            line = lines[i]
            
            # 检测void特化开始
            if 'template<>' in line and 'NAsyncTask<void>' in line:
                in_void_specialization = True
                fixed_lines.append(line)
                i += 1
                continue
            
            # 如果在void特化中，检查是否遇到重复的类定义
            if in_void_specialization and not void_specialization_complete:
                if 'class NAsyncTask : public NAsyncTaskBase' in line:
                    # 跳过重复的定义
                    i += 1
                    continue
                elif '};' in line and '// void特化版本' in lines[i-1] if i > 0 else False:
                    void_specialization_complete = True
                    fixed_lines.append(line)
                    i += 1
                    continue
            
            fixed_lines.append(line)
            i += 1
        
        # 写回文件
        with open(async_task_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(fixed_lines))
        
        print(f"修复了 {async_task_file} 中的重复类定义")

if __name__ == '__main__':
    print("开始修复关键的命名问题...")
    fix_critical_naming_issues()
    fix_duplicate_class_definitions()
    print("修复完成！") 