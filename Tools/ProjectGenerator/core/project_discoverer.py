"""
项目发现器

自动发现项目目录下的所有项目和构建文件
"""

import logging
from pathlib import Path
from typing import List, Optional

from Tools.ProjectGenerator.core.project_info import ProjectInfo, ProjectType
from Tools.ProjectGenerator.core.metadata_parser import MetadataParser
from Tools.ProjectGenerator.core.file_collector import FileCollector


logger = logging.getLogger(__name__)


class ProjectDiscoverer:
    """项目发现器"""
    
    def __init__(self, project_root: Path):
        """
        初始化项目发现器
        
        Args:
            project_root: 项目根目录
        """
        self.project_root = Path(project_root).resolve()
        self.source_dir = self.project_root / "Source"
        self.metadata_parser = MetadataParser()
        self.file_collector = FileCollector()
        
        if not self.source_dir.exists():
            raise ValueError(f"源代码目录不存在: {self.source_dir}")
    
    def DiscoverAllProjects(self) -> List[ProjectInfo]:
        """
        发现所有项目
        
        Returns:
            项目信息列表
        """
        projects = []
        
        logger.info(f"开始搜索项目目录: {self.source_dir}")
        
        # 搜索所有可能的项目目录
        for project_dir in self._FindProjectDirectories():
            try:
                project_info = self._AnalyzeProjectDirectory(project_dir)
                if project_info:
                    projects.append(project_info)
                    logger.info(f"发现项目: {project_info.name} ({project_info.project_type.value})")
            except Exception as e:
                logger.warning(f"分析项目目录失败 {project_dir}: {e}")
        
        logger.info(f"共发现 {len(projects)} 个项目")
        return projects
    
    def _FindProjectDirectories(self) -> List[Path]:
        """查找所有可能的项目目录"""
        project_dirs = []
        
        # 搜索包含构建文件的目录
        for build_file in self.source_dir.rglob("*.Build.cs"):
            project_dirs.append(build_file.parent.parent)
            
        for build_file in self.source_dir.rglob("*.Build.py"):
            project_dirs.append(build_file.parent.parent)
            
        # 搜索包含 .csproj 文件的目录
        for csproj_file in self.source_dir.rglob("*.csproj"):
            project_dirs.append(csproj_file.parent)
        
        # 去重并排序
        return sorted(set(project_dirs))
    
    def _AnalyzeProjectDirectory(self, project_dir: Path) -> Optional[ProjectInfo]:
        """
        分析项目目录
        
        Args:
            project_dir: 项目目录路径
            
        Returns:
            项目信息，如果不是有效项目则返回 None
        """
        project_name = project_dir.name
        
        # 确定项目类型和分组
        project_type = self._DetermineProjectType(project_dir)
        if not project_type:
            return None
            
        group_name = self._GetProjectGroup(project_dir)
        
        # 创建项目信息
        project_info = ProjectInfo(
            name=project_name,
            path=project_dir,
            project_type=project_type,
            group_name=group_name
        )
        
        # 解析构建元数据
        build_metadata = self._ParseBuildMetadata(project_dir)
        if build_metadata:
            project_info.build_metadata = build_metadata
            
        # 收集项目文件
        self.file_collector.CollectFiles(project_info, self.project_root)
        
        return project_info
    
    def _DetermineProjectType(self, project_dir: Path) -> Optional[ProjectType]:
        """确定项目类型"""
        # C# 项目
        if list(project_dir.glob("*.csproj")):
            # 检查是否包含 Main 方法
            for cs_file in project_dir.rglob("*.cs"):
                try:
                    content = cs_file.read_text(encoding='utf-8')
                    if 'static void Main(' in content or 'static async Task Main(' in content:
                        return ProjectType.CSHARP_EXECUTABLE
                except:
                    continue
            return ProjectType.CSHARP_LIBRARY
        
        # C++ 项目
        meta_dir = project_dir / "Meta"
        if meta_dir.exists():
            # 检查 Build 文件
            if list(meta_dir.glob("*.Build.cs")) or list(meta_dir.glob("*.Build.py")):
                # 检查是否包含 main 函数
                sources_dir = project_dir / "Sources"
                if sources_dir.exists():
                    for cpp_file in sources_dir.rglob("*Main.cpp"):
                        return ProjectType.EXECUTABLE
                    
                    for cpp_file in sources_dir.rglob("main.cpp"):
                        return ProjectType.EXECUTABLE
                        
                    # 检查文件内容中的 main 函数
                    for cpp_file in sources_dir.rglob("*.cpp"):
                        try:
                            content = cpp_file.read_text(encoding='utf-8')
                            if 'int main(' in content:
                                return ProjectType.EXECUTABLE
                        except:
                            continue
                
                return ProjectType.STATIC_LIBRARY
        
        return None
    
    def _GetProjectGroup(self, project_dir: Path) -> str:
        """获取项目分组"""
        try:
            # 获取相对于 Source 目录的路径
            relative_path = project_dir.relative_to(self.source_dir)
            parts = relative_path.parts
            
            if len(parts) > 0:
                return parts[0]  # 返回第一级目录名
        except ValueError:
            pass
        
        return "Other"
    
    def _ParseBuildMetadata(self, project_dir: Path) -> dict:
        """解析构建元数据"""
        meta_dir = project_dir / "Meta"
        if not meta_dir.exists():
            return {}
        
        # 尝试解析 Python 构建文件
        for py_file in meta_dir.glob("*.Build.py"):
            try:
                return self.metadata_parser.ParsePythonBuildFile(py_file)
            except Exception as e:
                logger.warning(f"解析 Python 构建文件失败 {py_file}: {e}")
        
        # 尝试解析 C# 构建文件  
        for cs_file in meta_dir.glob("*.Build.cs"):
            try:
                return self.metadata_parser.ParseCsharpBuildFile(cs_file)
            except Exception as e:
                logger.warning(f"解析 C# 构建文件失败 {cs_file}: {e}")
        
        return {}