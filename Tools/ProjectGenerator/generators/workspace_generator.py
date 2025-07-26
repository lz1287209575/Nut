"""
工作空间生成器

生成 XCode Workspace 和 Visual Studio Solution 文件
"""

import logging
from pathlib import Path
from typing import List, Dict, Any

from Tools.ProjectGenerator.generators.base_generator import BaseGenerator
from Tools.ProjectGenerator.core.project_info import ProjectInfo


logger = logging.getLogger(__name__)


class WorkspaceGenerator(BaseGenerator):
    """工作空间生成器"""
    
    @property
    def FileExtension(self) -> str:
        return ".xcworkspace"
    
    @property
    def GeneratorName(self) -> str:
        return "Workspace"
    
    def GenerateProject(self, project_info: ProjectInfo) -> Path:
        """工作空间生成器不生成单个项目"""
        raise NotImplementedError("工作空间生成器用于生成整体工作空间文件")
    
    def GenerateWorkspace(self, projects: List[ProjectInfo], workspace_type: str = "xcode") -> Path:
        """
        生成工作空间文件
        
        Args:
            projects: 项目列表
            workspace_type: 工作空间类型 ('xcode' 或 'vs')
            
        Returns:
            生成的工作空间文件路径
        """
        if workspace_type == "xcode":
            return self._GenerateXcodeWorkspace(projects)
        elif workspace_type == "vs":
            return self._GenerateVsSolution(projects)
        else:
            raise ValueError(f"不支持的工作空间类型: {workspace_type}")
    
    def _GenerateXcodeWorkspace(self, projects: List[ProjectInfo]) -> Path:
        """生成 XCode 工作空间"""
        workspace_path = self.project_root / "Nut.xcworkspace"
        workspace_data_path = workspace_path / "contents.xcworkspacedata"
        
        # 确保目录存在
        self._EnsureOutputDirectory(workspace_data_path)
        
        # 按分组组织项目
        groups = self._GroupProjects(projects)
        
        # 生成 XML 内容
        lines = [
            '<?xml version="1.0" encoding="UTF-8"?>',
            '<Workspace',
            '   version = "1.0">'
        ]
        
        for group_name, group_projects in groups.items():
            if not group_projects:
                continue
                
            # 过滤掉 C# 项目（XCode 不支持）
            cpp_projects = [p for p in group_projects if not p.is_csharp]
            if not cpp_projects:
                continue
            
            lines.append(f'   <Group')
            lines.append(f'      location = "container:"')
            lines.append(f'      name = "{group_name}">')
            
            for project in cpp_projects:
                # 确保路径使用正确的相对路径格式
                project_path = f"Projects/{group_name}/{project.name}.xcodeproj"
                lines.append(f'      <FileRef')
                lines.append(f'         location = "group:{project_path}">')
                lines.append(f'      </FileRef>')
            
            lines.append('   </Group>')
        
        lines.append('</Workspace>')
        
        # 写入文件
        content = '\n'.join(lines)
        workspace_data_path.write_text(content, encoding='utf-8')
        
        logger.info(f"生成 XCode 工作空间: {workspace_path}")
        return workspace_path
    
    def _GenerateVsSolution(self, projects: List[ProjectInfo]) -> Path:
        """生成 Visual Studio 解决方案"""
        solution_path = self.project_root / "Nut.sln"
        
        lines = [
            "Microsoft Visual Studio Solution File, Format Version 12.00",
            "# Visual Studio Version 17", 
            "VisualStudioVersion = 17.0.31903.59",
            "MinimumVisualStudioVersion = 10.0.40219.1"
        ]
        
        # 按分组组织项目
        groups = self._GroupProjects(projects)
        
        # 生成项目配置和嵌套项目数据
        project_configs = []
        nested_projects = []
        
        # 添加项目
        for group_name, group_projects in groups.items():
            if not group_projects:
                continue
                
            for project in group_projects:
                guid = self.uuid_generator.GenerateGuid(project.name)
                
                if project.is_csharp:
                    project_type_guid = "{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}"
                    project_file = f"Source/{project.path.relative_to(self.project_root / 'Source')}/{project.name}.csproj"
                else:
                    project_type_guid = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}" 
                    project_file = f"Projects/{group_name}/{project.name}.vcxproj"
                
                lines.append(f'Project("{project_type_guid}") = "{project.name}", "{project_file}", "{guid}"')
                lines.append("EndProject")
                
                # 添加项目配置（根据项目类型选择平台）
                if project.is_csharp:
                    # C# 项目使用 Any CPU
                    project_configs.extend([
                        f"{guid}.Debug|Any CPU.ActiveCfg = Debug|Any CPU",
                        f"{guid}.Debug|Any CPU.Build.0 = Debug|Any CPU", 
                        f"{guid}.Debug|x64.ActiveCfg = Debug|Any CPU",
                        f"{guid}.Debug|x64.Build.0 = Debug|Any CPU",
                        f"{guid}.Release|Any CPU.ActiveCfg = Release|Any CPU",
                        f"{guid}.Release|Any CPU.Build.0 = Release|Any CPU",
                        f"{guid}.Release|x64.ActiveCfg = Release|Any CPU",
                        f"{guid}.Release|x64.Build.0 = Release|Any CPU"
                    ])
                else:
                    # C++ 项目使用 x64
                    project_configs.extend([
                        f"{guid}.Debug|Any CPU.ActiveCfg = Debug|x64",
                        f"{guid}.Debug|x64.ActiveCfg = Debug|x64",
                        f"{guid}.Debug|x64.Build.0 = Debug|x64",
                        f"{guid}.Release|Any CPU.ActiveCfg = Release|x64",
                        f"{guid}.Release|x64.ActiveCfg = Release|x64",
                        f"{guid}.Release|x64.Build.0 = Release|x64"
                    ])
                
                # 添加嵌套项目
                group_guid = self.uuid_generator.GenerateGuid(f"Folder_{group_name}")
                nested_projects.append(f"{guid} = {group_guid}")
        
        # 添加文件夹
        for group_name in groups.keys():
            if groups[group_name]:  # 只添加非空分组
                group_guid = self.uuid_generator.GenerateGuid(f"Folder_{group_name}")
                lines.append(f'Project("{{2150E333-8FDC-42A3-9474-1A3956D46DE8}}") = "{group_name}", "{group_name}", "{group_guid}"')
                lines.append("EndProject")
        
        # 添加全局配置
        lines.extend([
            "Global",
            "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution",
            "\t\tDebug|Any CPU = Debug|Any CPU",
            "\t\tDebug|x64 = Debug|x64",
            "\t\tRelease|Any CPU = Release|Any CPU", 
            "\t\tRelease|x64 = Release|x64",
            "\tEndGlobalSection",
            "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution"
        ])
        
        for config in project_configs:
            lines.append(f"\t\t{config}")
        
        # 添加嵌套项目
        lines.extend([
            "\tEndGlobalSection",
            "\tGlobalSection(NestedProjects) = preSolution"
        ])
        
        for nested in nested_projects:
            lines.append(f"\t\t{nested}")
        
        lines.extend([
            "\tEndGlobalSection",
            "\tGlobalSection(SolutionProperties) = preSolution",
            "\t\tHideSolutionNode = FALSE",
            "\tEndGlobalSection",
            "EndGlobal"
        ])
        
        # 写入文件
        content = '\n'.join(lines)
        solution_path.write_text(content, encoding='utf-8')
        
        logger.info(f"生成 Visual Studio 解决方案: {solution_path}")
        return solution_path
    
    def _GroupProjects(self, projects: List[ProjectInfo]) -> Dict[str, List[ProjectInfo]]:
        """将项目按分组组织"""
        groups = {}
        
        for project in projects:
            group_name = project.group_name
            if group_name not in groups:
                groups[group_name] = []
            groups[group_name].append(project)
        
        # 按项目名称排序
        for group_projects in groups.values():
            group_projects.sort(key=lambda p: p.name)
        
        return groups