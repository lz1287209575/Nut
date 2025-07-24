"""
元数据解析器

解析项目的构建元数据文件（*.Build.py, *.Build.cs）
"""

import ast
import logging
import re
from pathlib import Path
from typing import Dict, Any, Optional


logger = logging.getLogger(__name__)


class MetadataParser:
    """元数据解析器"""
    
    def ParsePythonBuildFile(self, build_file: Path) -> Dict[str, Any]:
        """
        解析 Python 构建文件
        
        Args:
            build_file: Python 构建文件路径
            
        Returns:
            解析出的元数据字典
        """
        try:
            # 读取文件内容
            content = build_file.read_text(encoding='utf-8')
            
            # 解析 AST
            tree = ast.parse(content)
            
            # 查找 ServiceMeta 变量赋值
            for node in ast.walk(tree):
                if isinstance(node, ast.Assign):
                    for target in node.targets:
                        if isinstance(target, ast.Name) and target.id == 'ServiceMeta':
                            # 提取字典值
                            if isinstance(node.value, ast.Dict):
                                return self._ExtractDictFromAst(node.value)
            
            logger.warning(f"未找到 ServiceMeta 定义: {build_file}")
            return {}
            
        except Exception as e:
            logger.error(f"解析 Python 构建文件失败 {build_file}: {e}")
            return {}
    
    def ParseCsharpBuildFile(self, build_file: Path) -> Dict[str, Any]:
        """
        解析 C# 构建文件
        
        Args:
            build_file: C# 构建文件路径
            
        Returns:
            解析出的元数据字典
        """
        try:
            content = build_file.read_text(encoding='utf-8')
            
            # 提取类名
            class_match = re.search(r'class\s+(\w+)\s*:\s*NutTarget', content)
            if class_match:
                class_name = class_match.group(1)
                return {
                    'name': class_name.replace('Target', ''),
                    'type': 'csharp_target',
                    'class_name': class_name
                }
                
            logger.warning(f"未找到 NutTarget 派生类: {build_file}")
            return {}
            
        except Exception as e:
            logger.error(f"解析 C# 构建文件失败 {build_file}: {e}")
            return {}
    
    def _ExtractDictFromAst(self, dict_node: ast.Dict) -> Dict[str, Any]:
        """从 AST 字典节点提取 Python 字典"""
        result = {}
        
        for key_node, value_node in zip(dict_node.keys, dict_node.values):
            # 提取键
            if isinstance(key_node, ast.Str):
                key = key_node.s
            elif isinstance(key_node, ast.Constant) and isinstance(key_node.value, str):
                key = key_node.value
            else:
                continue
            
            # 提取值
            value = self._ExtractValueFromAst(value_node)
            if value is not None:
                result[key] = value
        
        return result
    
    def _ExtractValueFromAst(self, value_node: ast.AST) -> Any:
        """从 AST 节点提取值"""
        if isinstance(value_node, ast.Str):
            return value_node.s
        elif isinstance(value_node, ast.Constant):
            return value_node.value
        elif isinstance(value_node, ast.List):
            return [self._ExtractValueFromAst(item) for item in value_node.elts]
        elif isinstance(value_node, ast.Dict):
            return self._ExtractDictFromAst(value_node)
        elif isinstance(value_node, ast.Num):  # Python < 3.8 兼容性
            return value_node.n
        else:
            return None