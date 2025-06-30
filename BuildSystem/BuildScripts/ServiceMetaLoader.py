import importlib.util
from pathlib import Path

def LoadAllServiceMeta(microservices_dir: Path):
    services = {}
    for svc_dir in microservices_dir.iterdir():
        meta_file = svc_dir / "Meta" / f"{svc_dir.name}.Build.py"
        if meta_file.exists():
            spec = importlib.util.spec_from_file_location("ServiceMeta", meta_file)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            services[module.ServiceMeta["name"]] = module.ServiceMeta
    return services 