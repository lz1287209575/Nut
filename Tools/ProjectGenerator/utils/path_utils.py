"""
路径工具

提供跨平台的路径处理功能
"""

import os
import platform
from pathlib import Path
from typing import Optional


class PathUtils:
    """路径工具类"""
    
    @staticmethod
    def GetRelativePath(target: Path, base: Path) -> Path:
        """
        获取相对路径，支持跨平台
        
        Args:
            target: 目标路径
            base: 基准路径
            
        Returns:
            相对路径
        """
        try:
            return target.relative_to(base)
        except ValueError:
            # 如果无法计算相对路径，返回绝对路径
            return target.resolve()
    
    @staticmethod
    def NormalizePath(path: Path) -> str:
        """
        标准化路径格式
        
        Args:
            path: 路径对象
            
        Returns:
            标准化的路径字符串
        """
        # 在 Windows 上使用反斜杠，其他平台使用正斜杠
        if platform.system() == 'Windows':
            return str(path).replace('/', '\\')
        else:
            return str(path).replace('\\', '/')
    
    @staticmethod
    def IsSubdirectory(child: Path, parent: Path) -> bool:
        """
        检查是否为子目录
        
        Args:
            child: 子路径
            parent: 父路径
            
        Returns:
            如果 child 是 parent 的子目录返回 True
        """
        try:
            child.resolve().relative_to(parent.resolve())
            return True
        except ValueError:
            return False
    
    @staticmethod
    def FindCommonParent(paths: list[Path]) -> Optional[Path]:
        """
        找到多个路径的共同父目录
        
        Args:
            paths: 路径列表
            
        Returns:
            共同父目录，如果没有则返回 None
        """
        if not paths:
            return None
        
        if len(paths) == 1:
            return paths[0].parent
        
        # 转换为绝对路径
        abs_paths = [p.resolve() for p in paths]
        
        # 获取所有路径的部分
        path_parts = [list(p.parts) for p in abs_paths] 
        
        # 找到共同前缀
        common_parts = []
        min_length = min(len(parts) for parts in path_parts)
        
        for i in range(min_length):
            part = path_parts[0][i]
            if all(parts[i] == part for parts in path_parts):
                common_parts.append(part)
            else:
                break
        
        if common_parts:
            return Path(*common_parts)
        else:
            return None
    
    @staticmethod
    def EnsureDirectory(path: Path) -> Path:
        """
        确保目录存在
        
        Args:
            path: 目录路径
            
        Returns:
            目录路径
        """
        path.mkdir(parents=True, exist_ok=True)
        return path
    
    @staticmethod
    def GetFileExtensionType(file_path: Path) -> str:
        """
        根据文件扩展名获取文件类型
        
        Args:
            file_path: 文件路径
            
        Returns:
            文件类型标识符
        """
        suffix = file_path.suffix.lower()
        
        # C/C++ 文件类型映射
        cpp_types = {
            '.cpp': 'cpp',
            '.cxx': 'cpp', 
            '.cc': 'cpp',
            '.c': 'c',
            '.h': 'header',
            '.hpp': 'header',
            '.hxx': 'header',
            '.hh': 'header'
        }
        
        # 其他文件类型
        other_types = {
            '.cs': 'csharp',
            '.py': 'python',
            '.json': 'json',
            '.xml': 'xml',
            '.proto': 'protobuf',
            '.md': 'markdown',
            '.txt': 'text',
            '.yaml': 'yaml',
            '.yml': 'yaml'
        }
        
        return cpp_types.get(suffix) or other_types.get(suffix, 'unknown')
    
    @staticmethod
    def SafeFilename(name: str) -> str:
        """
        生成安全的文件名（移除非法字符）
        
        Args:
            name: 原始名称
            
        Returns:
            安全的文件名
        """
        # Windows 非法字符
        illegal_chars = '<>:"/\\|?*'
        
        # 替换非法字符为下划线
        safe_name = ''.join('_' if c in illegal_chars else c for c in name)
        
        # 移除首尾空格和点号
        safe_name = safe_name.strip(' .')
        
        # 确保不为空
        if not safe_name:
            safe_name = 'unnamed'
        
        return safe_name