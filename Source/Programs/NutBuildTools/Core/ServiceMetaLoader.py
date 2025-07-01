import importlib.util
from pathlib import Path

def LoadAllServiceMeta(parent_dir: Path):
    services = {}
    for svc_dir in parent_dir.iterdir():
        if not svc_dir.is_dir():
            continue
        # 1. 主Meta目录
        meta_dir = svc_dir / "Meta"
        if meta_dir.exists():
            for meta_file in meta_dir.glob("*.Build.py"):
                spec = importlib.util.spec_from_file_location("ServiceMeta", meta_file)
                module = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(module)
                if hasattr(module, "ServiceMeta"):
                    services[module.ServiceMeta["name"]] = module.ServiceMeta
        # 2. UnitTest/Meta目录
        unittest_meta_dir = svc_dir / "UnitTest" / "Meta"
        if unittest_meta_dir.exists():
            for meta_file in unittest_meta_dir.glob("*.Build.py"):
                spec = importlib.util.spec_from_file_location("ServiceMeta", meta_file)
                module = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(module)
                if hasattr(module, "ServiceMeta"):
                    services[module.ServiceMeta["name"]] = module.ServiceMeta
    return services 