import os
from pathlib import Path
from BuildSystem.BuildScripts.ServiceMetaLoader import LoadAllServiceMeta

class BuildEngine:
    def __init__(self):
        self.project_root = Path(__file__).parent.parent.parent
        self.services = LoadAllServiceMeta(self.project_root / "MicroServices")

    def ListServices(self):
        print("可用服务：")
        for svc in self.services:
            print(f"  - {svc['name']}")

    def GenerateService(self, service_name: str, port: int = None):
        """生成新服务"""
        from BuildSystem.BuildScripts.ServiceGenerator import ServiceGenerator
        generator = ServiceGenerator()
        return generator.GenerateService(service_name, port)

    # 预留Build/Clean/Package等方法
    # ...

if __name__ == "__main__":
    engine = BuildEngine()
    engine.ListServices() 