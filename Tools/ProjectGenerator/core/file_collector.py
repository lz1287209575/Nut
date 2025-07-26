"""
文件收集器

收集项目中的源文件、头文件、配置文件等
"""

import logging
from pathlib import Path
from typing import Set

from Tools.ProjectGenerator.core.project_info import ProjectInfo, FileGroup


logger = logging.getLogger(__name__)


class FileCollector:
    """文件收集器"""
    
    # 支持的文件扩展名
    HEADER_EXTENSIONS = {'.h', '.hpp', '.hxx', '.hh'}
    SOURCE_EXTENSIONS = {'.cpp', '.cxx', '.cc', '.c', '.cs'}
    CONFIG_EXTENSIONS = {'.json', '.xml', '.yaml', '.yml', '.ini'}
    META_EXTENSIONS = {'.py', '.cs', '.md', '.txt'}
    PROTO_EXTENSIONS = {'.proto'}
    
    def CollectFiles(self, project_info: ProjectInfo, project_root: Path):
        """
        收集项目文件
        
        Args:
            project_info: 项目信息对象
            project_root: 项目根目录
        """
        project_dir = project_info.path
        
        # 收集 Sources 目录下的文件
        sources_dir = project_dir / "Sources"
        if sources_dir.exists():
            self._CollectDirectoryFiles(
                sources_dir, project_info, project_root,
                {FileGroup.HEADERS, FileGroup.SOURCES}
            )
        
        # 收集 Meta 目录下的文件
        meta_dir = project_dir / "Meta"
        if meta_dir.exists():
            self._CollectDirectoryFiles(
                meta_dir, project_info, project_root,
                {FileGroup.META}
            )
        
        # 收集 Configs 目录下的文件
        configs_dir = project_dir / "Configs"
        if configs_dir.exists():
            self._CollectDirectoryFiles(
                configs_dir, project_info, project_root,
                {FileGroup.CONFIGS}
            )
        
        # 收集 Protos 目录下的文件 (作为META文件处理)
        protos_dir = project_dir / "Protos"
        if protos_dir.exists():
            self._CollectDirectoryFiles(
                protos_dir, project_info, project_root,
                {FileGroup.META}
            )
        
        # 收集项目根目录下的项目文件
        self._CollectProjectFiles(project_dir, project_info, project_root)
        
        logger.debug(f"项目 {project_info.name} 文件收集完成:")
        for group in FileGroup:
            count = len(project_info.files[group])
            if count > 0:
                logger.debug(f"  {group.value}: {count} 个文件")
    
    def _CollectDirectoryFiles(
        self, 
        directory: Path, 
        project_info: ProjectInfo, 
        project_root: Path,
        allowed_groups: Set[FileGroup]
    ):
        """收集指定目录下的文件"""
        for file_path in directory.rglob("*"):
            if not file_path.is_file():
                continue
            
            # 跳过隐藏文件
            if file_path.name.startswith('.'):
                continue
            
            # 确定文件分组
            file_extension = file_path.suffix.lower()
            file_group = self._DetermineFileGroup(file_extension)
            
            # 检查是否允许此分组
            if file_group not in allowed_groups:
                continue
            
            # 添加文件到项目
            project_info.AddFile(file_path, file_group, project_root)
    
    def _CollectProjectFiles(self, project_dir: Path, project_info: ProjectInfo, project_root: Path):
        """收集项目文件（如 .csproj）"""
        for file_path in project_dir.glob("*"):
            if not file_path.is_file():
                continue
            
            # 收集项目配置文件
            if file_path.suffix.lower() in {'.csproj', '.vcxproj', '.pbxproj'}:
                project_info.AddFile(file_path, FileGroup.META, project_root)
    
    def _DetermineFileGroup(self, file_extension: str) -> FileGroup:
        """根据文件扩展名确定文件分组"""
        if file_extension in self.HEADER_EXTENSIONS:
            return FileGroup.HEADERS
        elif file_extension in self.SOURCE_EXTENSIONS:
            return FileGroup.SOURCES
        elif file_extension in self.CONFIG_EXTENSIONS:
            return FileGroup.CONFIGS
        elif file_extension in self.PROTO_EXTENSIONS:
            return FileGroup.META
        else:
            return FileGroup.META