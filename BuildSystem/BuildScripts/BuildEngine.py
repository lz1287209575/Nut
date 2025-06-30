import sys
from pathlib import Path
from BuildSystem.BuildScripts.ServiceMetaLoader import LoadAllServiceMeta
from BuildSystem.BuildScripts.Compiler import BuildService, CleanService, PackageService

class BuildEngine:
    def __init__(self):
        self.project_root = Path(__file__).parent.parent.parent
        self.services = LoadAllServiceMeta(self.project_root / "MicroServices")

    def ListServices(self):
        print("可用服务：")
        for svc in self.services.values():
            print(f"  - {svc['name']}")

    def Build(self, service_name: str|None = None):
        if service_name:
            if service_name in self.services:
                print(f"正在编译服务: {service_name}")
                BuildService(self.services[service_name], self.project_root)
            else:
                print(f"未找到服务: {service_name}")
        else:
            print("正在编译所有服务...")
            for svc in self.services.values():
                BuildService(svc, self.project_root)

    def Clean(self, service_name: str|None = None):
        if service_name:
            if service_name in self.services:
                print(f"正在清理服务: {service_name}")
                CleanService(self.services[service_name], self.project_root)
            else:
                print(f"未找到服务: {service_name}")
        else:
            print("正在清理所有服务...")
            for svc in self.services.values():
                CleanService(svc, self.project_root)

    def Package(self, service_name: str|None = None):
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

    def GenerateService(self, service_name: str, port: int|None = None):
        """生成新服务"""
        from BuildSystem.BuildScripts.ServiceGenerator import ServiceGenerator
        generator = ServiceGenerator()
        return generator.GenerateService(service_name, port)

    # 预留Build/Clean/Package等方法
    # ...

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