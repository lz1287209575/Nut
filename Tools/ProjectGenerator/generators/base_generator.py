"""
生成器基类

定义所有项目文件生成器的通用接口和功能
"""

import logging
from abc import ABC, abstractmethod
from pathlib import Path
from typing import List, Optional

from Tools.ProjectGenerator.core.project_info import ProjectInfo
from Tools.ProjectGenerator.utils.uuid_generator import UuidGenerator


logger = logging.getLogger(__name__)


class BaseGenerator(ABC):
    """项目文件生成器基类"""
    
    def __init__(self, project_root: Path, template_dir: Optional[Path] = None):
        """
        初始化生成器
        
        Args:
            project_root: 项目根目录
            template_dir: 模板目录，如果为 None 则使用默认模板
        """
        self.project_root = Path(project_root).resolve()
        self.template_dir = template_dir or self._GetDefaultTemplateDir()
        self.uuid_generator = UuidGenerator()
        
        if not self.project_root.exists():
            raise ValueError(f"项目根目录不存在: {self.project_root}")
    
    @abstractmethod
    def GenerateProject(self, project_info: ProjectInfo) -> Path:
        """
        生成单个项目文件
        
        Args:
            project_info: 项目信息
            
        Returns:
            生成的项目文件路径
        """
        pass
    
    def GenerateProjects(self, projects: List[ProjectInfo]) -> List[Path]:
        """
        批量生成项目文件
        
        Args:
            projects: 项目信息列表
            
        Returns:
            生成的项目文件路径列表
        """
        generated_files = []
        
        for project_info in projects:
            try:
                project_file = self.GenerateProject(project_info)
                generated_files.append(project_file)
                logger.info(f"生成项目文件: {project_file}")
            except Exception as e:
                logger.error(f"生成项目 {project_info.name} 失败: {e}")
        
        return generated_files
    
    def _GetDefaultTemplateDir(self) -> Path:
        """获取默认模板目录"""
        return Path(__file__).parent.parent / "templates"
    
    def _EnsureOutputDirectory(self, output_path: Path) -> Path:
        """确保输出目录存在"""
        output_path.parent.mkdir(parents=True, exist_ok=True)
        return output_path
    
    def _GetRelativePath(self, target_path: Path, base_path: Path) -> Path:
        """获取相对路径"""
        try:
            return target_path.relative_to(base_path)
        except ValueError:
            # 如果无法计算相对路径，返回绝对路径
            return target_path
    
    @property
    @abstractmethod
    def FileExtension(self) -> str:
        """项目文件扩展名"""
        pass
    
    @property
    @abstractmethod
    def GeneratorName(self) -> str:
        """生成器名称"""
        pass