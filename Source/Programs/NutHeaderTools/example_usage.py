#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
NutHeaderTools 使用示例
演示如何使用头文件处理工具
"""

import sys
from pathlib import Path

# 添加脚本目录到路径
script_dir = Path(__file__).parent / "HeaderScripts"
sys.path.insert(0, str(script_dir))

from HeaderProcessor import HeaderProcessor
from HeaderAnalyzer import HeaderAnalyzer


def example_basic_usage():
    """基本使用示例"""
    print("=== NutHeaderTools 基本使用示例 ===\n")
    
    # 创建处理器实例
    processor = HeaderProcessor(".")
    
    # 扫描头文件
    print("1. 扫描头文件...")
    headers = processor.scan_headers()
    print(f"   找到 {len(headers)} 个头文件")
    for header in sorted(headers):
        print(f"   - {header}")
    
    # 分析依赖
    print("\n2. 分析依赖关系...")
    dependencies = processor.analyze_dependencies()
    print(f"   分析了 {len(dependencies)} 个文件的依赖关系")
    
    # 验证头文件
    print("\n3. 验证头文件...")
    issues = processor.validate_headers()
    total_issues = sum(len(issue_list) for issue_list in issues.values())
    print(f"   发现 {total_issues} 个问题")
    
    if total_issues > 0:
        for header, header_issues in issues.items():
            if header_issues:
                print(f"   {header}:")
                for issue in header_issues:
                    print(f"     - {issue}")


def example_advanced_analysis():
    """高级分析示例"""
    print("\n=== NutHeaderTools 高级分析示例 ===\n")
    
    # 创建分析器实例
    analyzer = HeaderAnalyzer(".")
    
    # 运行完整分析
    print("运行完整分析...")
    results = analyzer.run_full_analysis()
    
    # 显示摘要
    analyzer.print_summary()
    
    # 导出结果
    print("\n导出分析结果...")
    analyzer.export_analysis("header_analysis_example.json")
    print("结果已导出到 header_analysis_example.json")


def example_custom_scan():
    """自定义扫描示例"""
    print("\n=== NutHeaderTools 自定义扫描示例 ===\n")
    
    # 创建处理器实例
    processor = HeaderProcessor(".")
    
    # 自定义扫描目录
    custom_dirs = ['Source/Runtime/ServiceAllocate', 'Source/Runtime/LibNut']
    print(f"自定义扫描目录: {custom_dirs}")
    
    headers = processor.scan_headers(custom_dirs)
    print(f"在指定目录中找到 {len(headers)} 个头文件")
    
    for header in sorted(headers):
        print(f"  {header}")


if __name__ == "__main__":
    print("NutHeaderTools 使用示例")
    print("=" * 50)
    
    try:
        example_basic_usage()
        example_advanced_analysis()
        example_custom_scan()
        
        print("\n" + "=" * 50)
        print("示例执行完成！")
        
    except Exception as e:
        print(f"执行示例时出错: {e}")
        sys.exit(1) 