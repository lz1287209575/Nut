#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
NutHeaderTools - 头文件处理工具集
主入口脚本
"""

import sys
import os
from pathlib import Path

# 添加脚本目录到路径
script_dir = Path(__file__).parent / "HeaderScripts"
sys.path.insert(0, str(script_dir))

from HeaderProcessor import HeaderProcessor
from HeaderAnalyzer import HeaderAnalyzer


def show_usage():
    """显示使用说明"""
    print("""
NutHeaderTools - 头文件处理工具集

用法:
    python HeaderTools.py <命令> [选项]

命令:
    scan      - 扫描项目头文件
    analyze   - 分析头文件依赖和问题
    validate  - 验证头文件有效性
    graph     - 生成依赖关系图
    optimize  - 优化头文件包含

选项:
    --project-root <路径>  - 项目根目录 (默认: 当前目录)
    --output <文件>        - 输出文件路径
    --summary              - 只显示摘要信息

示例:
    python HeaderTools.py scan --project-root /path/to/project
    python HeaderTools.py analyze --output analysis.json
    python HeaderTools.py validate --summary
""")


def main():
    if len(sys.argv) < 2:
        show_usage()
        return
    
    command = sys.argv[1]
    
    # 解析参数
    project_root = "."
    output_file = None
    summary_only = False
    
    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == "--project-root" and i + 1 < len(sys.argv):
            project_root = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == "--output" and i + 1 < len(sys.argv):
            output_file = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == "--summary":
            summary_only = True
            i += 1
        else:
            i += 1
    
    try:
        if command == "scan":
            processor = HeaderProcessor(project_root)
            headers = processor.scan_headers()
            print(f"扫描到 {len(headers)} 个头文件:")
            for header in sorted(headers):
                print(f"  {header}")
                
        elif command == "analyze":
            analyzer = HeaderAnalyzer(project_root)
            results = analyzer.run_full_analysis()
            
            if summary_only:
                analyzer.print_summary()
            else:
                if output_file:
                    analyzer.export_analysis(output_file)
                analyzer.print_summary()
                
        elif command == "validate":
            processor = HeaderProcessor(project_root)
            processor.scan_headers()
            issues = processor.validate_headers()
            
            if summary_only:
                total_issues = sum(len(issue_list) for issue_list in issues.values())
                print(f"发现 {total_issues} 个问题")
            else:
                for header, header_issues in issues.items():
                    if header_issues:
                        print(f"\n{header}:")
                        for issue in header_issues:
                            print(f"  - {issue}")
                            
        elif command == "graph":
            processor = HeaderProcessor(project_root)
            processor.scan_headers()
            processor.analyze_dependencies()
            
            output = output_file or "header_dependencies.dot"
            processor.generate_dependency_graph(output)
            print(f"依赖关系图已生成: {output}")
            
        elif command == "optimize":
            print("头文件优化功能正在开发中...")
            
        else:
            print(f"未知命令: {command}")
            show_usage()
            
    except Exception as e:
        print(f"错误: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main() 