"""
项目信息数据结构

定义项目元数据和文件组织结构
"""

from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import List, Dict, Optional


class ProjectType(Enum):
    """项目类型枚举"""
    EXECUTABLE = "executable"
    STATIC_LIBRARY = "static_library"
    DYNAMIC_LIBRARY = "dynamic_library"
    CSHARP_EXECUTABLE = "csharp_executable"
    CSHARP_LIBRARY = "csharp_library"


class FileGroup(Enum):
    """文件分组枚举"""
    HEADERS = "Headers"
    SOURCES = "Sources" 
    META = "Meta"
    CONFIGS = "Configs"


@dataclass
class FileInfo:
    """文件信息"""
    path: Path
    relative_path: Path
    group: FileGroup
    file_type: str
    
    @property
    def name(self) -> str:
        """获取文件名"""
        return self.path.name


@dataclass 
class ProjectInfo:
    """项目信息数据类"""
    name: str
    path: Path
    project_type: ProjectType
    group_name: str = "Other"
    
    # 文件组织
    files: Dict[FileGroup, List[FileInfo]] = field(default_factory=dict)
    
    # 构建配置
    include_dirs: List[Path] = field(default_factory=list)
    dependencies: List[str] = field(default_factory=list)
    
    # 元数据
    build_metadata: Dict = field(default_factory=dict)
    
    def __post_init__(self):
        """初始化后处理"""
        # 确保所有文件组都存在
        for group in FileGroup:
            if group not in self.files:
                self.files[group] = []
    
    def AddFile(self, file_path: Path, group: FileGroup, project_root: Path):
        """添加文件到项目"""
        if not file_path.exists():
            return False
            
        # 确定文件类型
        file_type = self._GetFileType(file_path)
        relative_path = file_path.relative_to(project_root)
        
        file_info = FileInfo(
            path=file_path,
            relative_path=relative_path,
            group=group,
            file_type=file_type
        )
        
        self.files[group].append(file_info)
        return True
    
    def GetAllFiles(self) -> List[FileInfo]:
        """获取所有文件"""
        all_files = []
        for group in FileGroup:
            all_files.extend(self.files[group])
        return all_files
    
    def GetSourceFiles(self) -> List[FileInfo]:
        """获取源文件"""
        return self.files[FileGroup.SOURCES]
    
    def GetHeaderFiles(self) -> List[FileInfo]:
        """获取头文件"""
        return self.files[FileGroup.HEADERS]
    
    @staticmethod
    def _GetFileType(file_path: Path) -> str:
        """根据文件扩展名确定文件类型"""
        suffix = file_path.suffix.lower()
        
        type_map = {
            '.h': 'sourcecode.c.h',
            '.hpp': 'sourcecode.cpp.h', 
            '.hxx': 'sourcecode.cpp.h',
            '.cpp': 'sourcecode.cpp.cpp',
            '.cxx': 'sourcecode.cpp.cpp',
            '.cc': 'sourcecode.cpp.cpp',
            '.c': 'sourcecode.c.c',
            '.cs': 'sourcecode.cs',
            '.py': 'text.script.python',
            '.json': 'text.json',
            '.xml': 'text.xml',
            '.proto': 'text',
            '.md': 'text',
            '.txt': 'text.plain'
        }
        
        return type_map.get(suffix, 'text')
    
    @property
    def is_executable(self) -> bool:
        """是否为可执行项目"""
        return self.project_type in [
            ProjectType.EXECUTABLE,
            ProjectType.CSHARP_EXECUTABLE
        ]
    
    @property
    def is_library(self) -> bool:
        """是否为库项目"""
        return self.project_type in [
            ProjectType.STATIC_LIBRARY,
            ProjectType.DYNAMIC_LIBRARY,
            ProjectType.CSHARP_LIBRARY
        ]
    
    @property
    def is_csharp(self) -> bool:
        """是否为C#项目"""
        return self.project_type in [
            ProjectType.CSHARP_EXECUTABLE,
            ProjectType.CSHARP_LIBRARY
        ]
