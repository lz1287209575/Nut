#!/usr/bin/env python3
"""
ProjectGenerator 工具主入口

这是 Nut 项目文件生成器工具的统一入口点
"""

import sys
from pathlib import Path

# 添加项目根目录到 Python 路径
tool_dir = Path(__file__).parent
project_root = tool_dir.parent.parent
sys.path.insert(0, str(project_root))

# 导入并运行 CLI
from Tools.ProjectGenerator.cli import main

if __name__ == '__main__':
    main()