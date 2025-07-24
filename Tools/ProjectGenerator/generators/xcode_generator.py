"""
XCode 项目生成器

生成 XCode 项目文件 (.xcodeproj)
"""

import logging
from pathlib import Path
from typing import Dict, Any

from Tools.ProjectGenerator.generators.base_generator import BaseGenerator
from Tools.ProjectGenerator.core.project_info import ProjectInfo, FileGroup, ProjectType
from Tools.ProjectGenerator.utils.xml_builder import XmlBuilder


logger = logging.getLogger(__name__)


class XCodeProjectGenerator(BaseGenerator):
    """XCode 项目生成器"""
    
    @property
    def FileExtension(self) -> str:
        return ".xcodeproj"
    
    @property
    def GeneratorName(self) -> str:
        return "XCode"
    
    def GenerateProject(self, project_info: ProjectInfo) -> Path:
        """生成 XCode 项目文件"""
        if project_info.is_csharp:
            logger.warning(f"跳过 C# 项目: {project_info.name}")
            return None
        
        # 创建项目目录
        projects_dir = self.project_root / "Projects" / project_info.group_name
        project_dir = projects_dir / f"{project_info.name}.xcodeproj"
        
        # 确保项目目录存在
        project_dir.mkdir(parents=True, exist_ok=True)
        
        # 生成 project.pbxproj 文件
        pbxproj_path = project_dir / "project.pbxproj"
        self._GeneratePbxproj(project_info, pbxproj_path)
        
        logger.info(f"生成 XCode 项目: {project_dir}")
        return project_dir
    
    def _GeneratePbxproj(self, project_info: ProjectInfo, output_path: Path):
        """生成 project.pbxproj 文件"""
        # 生成所需的 UUID
        uuids = self._GenerateUuids(project_info)
        
        # 构建项目数据
        project_data = self._BuildProjectData(project_info, uuids)
        
        # 使用 XML 构建器生成文件
        xml_builder = XmlBuilder()
        content = xml_builder.BuildPbxproj(project_data)
        
        # 写入文件
        output_path.write_text(content, encoding='utf-8')
    
    def _GenerateUuids(self, project_info: ProjectInfo) -> Dict[str, Any]:
        """生成项目所需的所有 UUID"""
        uuids = {
            'project': self.uuid_generator.generate(),
            'main_group': self.uuid_generator.generate(),
            'target': self.uuid_generator.generate(),
            'product_ref': self.uuid_generator.generate(),
            'build_phase_sources': self.uuid_generator.generate(),
            'config_list_project': self.uuid_generator.generate(),
            'config_list_target': self.uuid_generator.generate(),
            'config_debug_project': self.uuid_generator.generate(),
            'config_release_project': self.uuid_generator.generate(),
            'config_debug_target': self.uuid_generator.generate(),
            'config_release_target': self.uuid_generator.generate(),
        }
        
        # 为每个文件组生成 UUID
        for group in FileGroup:
            uuids[f'group_{group.value.lower()}'] = self.uuid_generator.generate()
        
        # 为每个文件生成 UUID
        uuids['file_refs'] = {}
        uuids['build_files'] = {}
        
        all_files = project_info.GetAllFiles()
        for i, file_info in enumerate(all_files):
            file_key = f"file_{i}"
            uuids['file_refs'][file_key] = self.uuid_generator.generate()
            
            # 只为源文件生成构建文件 UUID
            if file_info.group == FileGroup.SOURCES:
                uuids['build_files'][file_key] = self.uuid_generator.generate()
        
        return uuids
    
    def _BuildProjectData(self, project_info: ProjectInfo, uuids: Dict[str, Any]) -> Dict[str, Any]:
        """构建项目数据结构"""
        all_files = project_info.GetAllFiles()
        
        # 确定产品类型
        if project_info.is_executable:
            product_type = "com.apple.product-type.tool"
        else:
            product_type = "com.apple.product-type.library.static"
        
        # 构建文件引用数据
        file_refs_data = []
        project_output_dir = self.project_root / "Projects" / project_info.group_name
        
        for i, file_info in enumerate(all_files):
            file_key = f"file_{i}"
            
            # 计算相对于项目文件所在目录的路径
            # 项目文件在 Projects/GroupName/ProjectName.xcodeproj
            # 需要从该目录返回到项目根目录，然后到达实际文件
            try:
                relative_path = file_info.path.relative_to(project_output_dir)
            except ValueError:
                # 如果文件不在项目输出目录下，计算相对路径
                # 从 Projects/GroupName 到项目根目录需要 ../../
                # 然后加上从项目根目录到文件的相对路径
                root_to_file = file_info.path.relative_to(self.project_root)
                relative_path = Path("../../") / root_to_file
            
            file_refs_data.append({
                'uuid': uuids['file_refs'][file_key],
                'name': file_info.name,
                'path': str(relative_path),
                'file_type': file_info.file_type,
                'source_tree': '<group>'
            })
        
        # 构建构建文件数据
        build_files_data = []
        for i, file_info in enumerate(all_files):
            file_key = f"file_{i}"
            if file_info.group == FileGroup.SOURCES:
                build_files_data.append({
                    'uuid': uuids['build_files'][file_key],
                    'file_ref': uuids['file_refs'][file_key],
                    'file_name': str(file_info.relative_path)
                })
        
        # 构建文件组数据
        groups_data = []
        for group in FileGroup:
            group_files = project_info.files[group]
            if not group_files:
                continue
            
            children = []
            for i, file_info in enumerate(all_files):
                if file_info.group == group:
                    file_key = f"file_{i}"
                    children.append({
                        'uuid': uuids['file_refs'][file_key],
                        'name': file_info.name
                    })
            
            groups_data.append({
                'uuid': uuids[f'group_{group.value.lower()}'],
                'name': group.value,
                'children': children
            })
        
        return {
            'project_name': project_info.name,
            'product_type': product_type,
            'uuids': uuids,
            'file_refs': file_refs_data,
            'build_files': build_files_data,
            'groups': groups_data,
            'main_group_children': [
                {'uuid': uuids[f'group_{group.value.lower()}'], 'name': group.value}
                for group in FileGroup if project_info.files[group]
            ]
        }