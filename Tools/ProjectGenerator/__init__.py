"""
Nut 项目文件生成器

工程化的项目文件生成系统，支持多种 IDE 格式：
- XCode 项目文件 (.xcodeproj)
- Visual Studio 项目文件 (.vcxproj/.sln)
- Workspace 文件 (.xcworkspace)
"""

__version__ = "1.0.0"
__author__ = "Nut Team"

from Tools.ProjectGenerator.core.project_info import ProjectInfo, ProjectType
from Tools.ProjectGenerator.generators.xcode_generator import XCodeProjectGenerator
from Tools.ProjectGenerator.generators.vcxproj_generator import VcxprojGenerator
from Tools.ProjectGenerator.generators.workspace_generator import WorkspaceGenerator

__all__ = [
    "ProjectInfo",
    "ProjectType", 
    "XCodeProjectGenerator",
    "VcxprojGenerator",
    "WorkspaceGenerator"
]