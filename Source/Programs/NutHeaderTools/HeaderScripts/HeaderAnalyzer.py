#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
头文件分析工具
提供详细的头文件分析功能
"""

import json
import argparse
from pathlib import Path
from typing import Dict, List, Set
from HeaderProcessor import HeaderProcessor


class HeaderAnalyzer:
    """头文件分析器"""
    
    def __init__(self, project_root: str):
        self.processor = HeaderProcessor(project_root)
        self.analysis_results = {}
        
    def run_full_analysis(self) -> Dict:
        """运行完整分析"""
        print("开始头文件分析...")
        
        # 扫描头文件
        headers = self.processor.scan_headers()
        print(f"扫描到 {len(headers)} 个头文件")
        
        # 分析依赖
        dependencies = self.processor.analyze_dependencies()
        
        # 验证头文件
        issues = self.processor.validate_headers()
        
        # 统计信息
        stats = self._calculate_statistics(headers, dependencies, issues)
        
        self.analysis_results = {
            'headers': [str(h) for h in headers],
            'dependencies': {str(k): [str(v) for v in vs] for k, vs in dependencies.items()},
            'issues': {str(k): v for k, v in issues.items()},
            'statistics': stats
        }
        
        return self.analysis_results
    
    def _calculate_statistics(self, headers: Set[Path], 
                            dependencies: Dict[Path, Set[Path]], 
                            issues: Dict[Path, List[str]]) -> Dict:
        """计算统计信息"""
        total_headers = len(headers)
        total_dependencies = sum(len(deps) for deps in dependencies.values())
        total_issues = sum(len(issue_list) for issue_list in issues.values())
        
        # 按目录统计
        dir_stats = {}
        for header in headers:
            dir_name = header.parent.name
            if dir_name not in dir_stats:
                dir_stats[dir_name] = {'count': 0, 'issues': 0}
            dir_stats[dir_name]['count'] += 1
            if header in issues:
                dir_stats[dir_name]['issues'] += len(issues[header])
        
        return {
            'total_headers': total_headers,
            'total_dependencies': total_dependencies,
            'total_issues': total_issues,
            'average_dependencies_per_header': total_dependencies / total_headers if total_headers > 0 else 0,
            'directory_statistics': dir_stats
        }
    
    def export_analysis(self, output_file: str):
        """导出分析结果"""
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(self.analysis_results, f, indent=2, ensure_ascii=False)
        print(f"分析结果已导出到: {output_file}")
    
    def print_summary(self):
        """打印分析摘要"""
        if not self.analysis_results:
            print("请先运行分析")
            return
            
        stats = self.analysis_results['statistics']
        print("\n=== 头文件分析摘要 ===")
        print(f"总头文件数: {stats['total_headers']}")
        print(f"总依赖数: {stats['total_dependencies']}")
        print(f"平均每个头文件的依赖数: {stats['average_dependencies_per_header']:.2f}")
        print(f"总问题数: {stats['total_issues']}")
        
        print("\n按目录统计:")
        for dir_name, dir_stats in stats['directory_statistics'].items():
            print(f"  {dir_name}: {dir_stats['count']} 个文件, {dir_stats['issues']} 个问题")


def main():
    parser = argparse.ArgumentParser(description='头文件分析工具')
    parser.add_argument('project_root', help='项目根目录路径')
    parser.add_argument('--output', '-o', help='输出文件路径', default='header_analysis.json')
    parser.add_argument('--summary', '-s', action='store_true', help='只显示摘要')
    
    args = parser.parse_args()
    
    analyzer = HeaderAnalyzer(args.project_root)
    results = analyzer.run_full_analysis()
    
    if args.summary:
        analyzer.print_summary()
    else:
        analyzer.export_analysis(args.output)
        analyzer.print_summary()


if __name__ == "__main__":
    main() 