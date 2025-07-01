#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
from pathlib import Path

# 添加项目根目录到Python路径
project_root = os.curdir
sys.path.insert(0, str(project_root))

from Core.Compiler import BuildService, CleanService, PackageService
from Core.MetaLoader import MetaLoader

class BuildEngine:
    def __init__(self):
        self.project_root = Path(os.curdir)
        self.services = {}
        
        self.meta_loaders = MetaLoader()

    def ListServices(self):
        self.services = self.meta_loaders.LoadMetas()

        print("可用服务：")
        for svc, meta in self.services.keys():
            print(f"  - {svc}")

    def Build(self, service_name: str|None = None):
        self.services = self.meta_loaders.LoadMetas()
        if service_name:
            if service_name in self.services:
                print(f"正在编译服务: {service_name}")
                BuildService(self.services[service_name])
            else:
                print(f"未找到服务: {service_name}")
        else:
            print("正在编译所有服务...")
            for svc in self.services.values():
                BuildService(svc)

    def Clean(self, service_name: str|None = None):
        self.services = self.meta_loaders.LoadMetas()
        if service_name:
            if service_name in self.services:
                print(f"正在清理服务: {service_name}")
                CleanService(self.services[service_name])
            else:
                print(f"未找到服务: {service_name}")
        else:
            print("正在清理所有服务...")
            for svc in self.services.values():
                CleanService(svc)

    def Package(self, service_name: str|None = None):
        self.services = self.meta_loaders.LoadMetas()
        if service_name:
            if service_name in self.services:
                print(f"正在打包服务: {service_name}")
                PackageService(self.services[service_name], self.project_root)
            else:
                print(f"未找到服务: {service_name}")
        else:
            print("正在打包所有服务...")
            for svc in self.services.values():
                PackageService(svc, self.project_root)

if __name__ == "__main__":
    engine = BuildEngine()
    if len(sys.argv) > 1:
        cmd = sys.argv[1].lower()
        arg = sys.argv[2] if len(sys.argv) > 2 else None
        if cmd == "build":
            engine.Build(arg)
        elif cmd == "clean":
            engine.Clean(arg)
        elif cmd == "package":
            engine.Package(arg)
        else:
            engine.ListServices()
    else:
        engine.ListServices() 