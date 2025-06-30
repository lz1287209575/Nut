#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Nut 服务生成器
快速生成新服务的基础框架代码
"""

import os
import sys
import argparse
import json
from pathlib import Path
from typing import Dict, List
import platform

# 设置控制台输出编码
if platform.system() == "Windows":
    import codecs
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout.detach())
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr.detach())

class ServiceGenerator:
    def __init__(self):
        self.project_root = Path(__file__).parent.parent.parent
        self.template_dir = self.project_root / "BuildSystem" / "Templates"
        self.services_dir = self.project_root / "MicroServices"
        
    def GenerateService(self, service_name: str, port: int = None) -> bool:
        """生成新服务"""
        print(f"正在生成服务: {service_name}")
        
        # 创建服务目录结构
        service_dir = self.services_dir / service_name
        self.CreateDirectoryStructure(service_dir)
        
        # 生成源文件
        self.GenerateSourceFiles(service_dir, service_name)
        
        # 生成配置文件
        self.GenerateConfigFiles(service_dir, service_name, port)
        
        # 生成协议文件
        self.GenerateProtoFiles(service_dir, service_name)
        
        # 生成Meta文件
        self.GenerateMetaFiles(service_dir, service_name, port)
        
        print(f"服务 {service_name} 生成完成!")
        print(f"目录: {service_dir}")
        print(f"端口: {port or self.GetNextAvailablePort()}")
        return True
    
    def CreateDirectoryStructure(self, service_dir: Path):
        """创建目录结构"""
        dirs = ["Sources", "Configs", "Protos", "Meta"]
        for dir_name in dirs:
            (service_dir / dir_name).mkdir(parents=True, exist_ok=True)
    
    def GenerateSourceFiles(self, service_dir: Path, service_name: str):
        """生成源文件"""
        main_cpp = service_dir / "Sources" / f"{service_name}Main.cpp"
        main_cpp.write_text(self.GetMainCppTemplate(service_name), encoding='utf-8')
    
    def GenerateConfigFiles(self, service_dir: Path, service_name: str, port: int):
        """生成配置文件"""
        if port is None:
            port = self.GetNextAvailablePort()
        
        config_json = service_dir / "Configs" / f"{service_name}Config.json"
        config_content = self.GetConfigTemplate(service_name, port)
        config_json.write_text(config_content, encoding='utf-8')
    
    def GenerateProtoFiles(self, service_dir: Path, service_name: str):
        """生成协议文件"""
        proto_file = service_dir / "Protos" / f"{service_name.lower()}.proto"
        proto_content = self.GetProtoTemplate(service_name)
        proto_file.write_text(proto_content, encoding='utf-8')
    
    def GenerateMetaFiles(self, service_dir: Path, service_name: str, port: int):
        """生成Meta文件"""
        # Build.py
        build_py = service_dir / "Meta" / f"{service_name}.Build.py"
        build_content = self.GetBuildTemplate(service_name)
        build_py.write_text(build_content, encoding='utf-8')
        
        # Docker.py
        docker_py = service_dir / "Meta" / f"{service_name}.Docker.py"
        docker_content = self.GetDockerTemplate(service_name, port)
        docker_py.write_text(docker_content, encoding='utf-8')
        
        # Kubernetes.py
        k8s_py = service_dir / "Meta" / f"{service_name}.Kubernetes.py"
        k8s_content = self.GetK8STemplate(service_name, port)
        k8s_py.write_text(k8s_content, encoding='utf-8')
    
    def GetNextAvailablePort(self) -> int:
        """获取下一个可用端口"""
        base_port = 50051
        used_ports = set()
        
        for service_dir in self.services_dir.iterdir():
            if service_dir.is_dir():
                config_file = service_dir / "Configs" / f"{service_dir.name}Config.json"
                if config_file.exists():
                    try:
                        with open(config_file, 'r', encoding='utf-8') as f:
                            config = json.load(f)
                            used_ports.add(config.get("listen_port", 0))
                    except:
                        pass
        
        port = base_port
        while port in used_ports:
            port += 1
        return port
    
    # 模板方法
    def GetMainCppTemplate(self, service_name: str) -> str:
        return f'''#include <iostream>

int main() {{
    std::cout << "{service_name} 启动成功" << std::endl;
    // TODO: 初始化服务、加载配置、启动主循环等
    return 0;
}}
'''
    
    def GetConfigTemplate(self, service_name: str, port: int) -> str:
        return f'''{{
    "service_name": "{service_name}",
    "listen_port": {port},
    "log_level": "info"
}}
'''
    
    def GetProtoTemplate(self, service_name: str) -> str:
        service_lower = service_name.lower()
        return f'''syntax = "proto3";

package {service_lower};

message {service_name}InitRequest {{
    string service_name = 1;
}}

message {service_name}InitResponse {{
    bool success = 1;
    string message = 2;
}}
'''
    
    def GetBuildTemplate(self, service_name: str) -> str:
        return f'''ServiceMeta = {{
    "name": "{service_name}",
    "sources": [
        "../Sources/{service_name}Main.cpp"
    ],
    "include_dirs": [
        "../Sources"
    ],
    "proto_files": [
        "../Protos/{service_name.lower()}.proto"
    ],
    "config_files": [
        "../Configs/{service_name}Config.json"
    ],
    "dependencies": [
        "protobuf"
    ],
    "output": "{service_name}Main"
}}
'''
    
    def GetDockerTemplate(self, service_name: str, port: int) -> str:
        return f'''DockerMeta = {{
    "base_image": "ubuntu:22.04",
    "copy_files": [
        "../Sources/{service_name}Main",
        "../Configs/{service_name}Config.json"
    ],
    "expose_ports": [{port}],
    "entrypoint": ["./{service_name}Main", "--config", "{service_name}Config.json"]
}}
'''
    
    def GetK8STemplate(self, service_name: str, port: int) -> str:
        service_lower = service_name.lower()
        return f'''K8SMeta = {{
    "deployment_name": "{service_lower}-service",
    "replicas": 2,
    "container_port": {port},
    "env": {{
        "CONFIG_PATH": "/app/{service_name}Config.json"
    }}
}}
'''

def Main():
    """主函数"""
    parser = argparse.ArgumentParser(description="Nut 服务生成器")
    parser.add_argument("service_name", help="服务名称")
    parser.add_argument("--port", "-p", type=int, help="监听端口")
    
    args = parser.parse_args()
    
    generator = ServiceGenerator()
    success = generator.GenerateService(args.service_name, args.port)
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(Main()) 