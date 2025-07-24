"""
核心模块

包含项目发现、元数据解析和文件收集等核心功能
"""

from Tools.ProjectGenerator.core.project_info import ProjectInfo, ProjectType, FileGroup
from Tools.ProjectGenerator.core.project_discoverer import ProjectDiscoverer
from Tools.ProjectGenerator.core.metadata_parser import MetadataParser
from Tools.ProjectGenerator.core.file_collector import FileCollector

__all__ = [
    "ProjectInfo",
    "ProjectType", 
    "FileGroup",
    "ProjectDiscoverer",
    "MetadataParser",
    "FileCollector"
]