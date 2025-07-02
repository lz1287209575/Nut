#! /usr/bin/env python3
# -*- coding: utf-8 -*-


from enum import Enum
import os
from pathlib import Path
from typing import Dict


class Meta:

    def __init__(self, name: str, meta_file: Path):
        self.name = name
        self.meta_file = meta_file

    def Load(self):
        raise NotImplementedError("Meta.Load is not implemented")

    @staticmethod
    def LoadMeta(meta_file: Path):
        import importlib
        import importlib.util
        import inspect
        print("================", meta_file.absolute().as_posix())
        spec = importlib.util.find_spec(meta_file.absolute().as_posix())
        if not spec:
            print(f"未找到 {meta_file.name} 对应的模块 Module: {meta_file.absolute().as_posix()}")
            return None
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        meta_classes = inspect.getmembers(module, inspect.isclass)

        for meta_class in meta_classes:
            if issubclass(meta_class[1], Meta):
                return meta_class[1](meta_class[1].__name__.replace("Meta", ""), meta_file)

        raise ValueError(f"未找到 {meta_file.name} 对应的 Meta 类")


class BuildMeta(Meta):

    def __init__(self, name: str, meta_file: Path):
        super().__init__(name, meta_file)
        self.sources_dir = meta_file.parent.parent   / "Sources"
        self.protos_dir = meta_file.parent.parent / "Protos"
        self.output_dir = meta_file.parent.parent / "Build"
        self.intermediate_dir = meta_file.parent.parent / "Intermediate"

        self.extra_include_dirs = []
        self.extra_link_dirs = []
        self.extra_link_libs = []
        self.build_type = BuildType.STATIC_LIB

        self.output_dir.mkdir(exist_ok=True)
        self.intermediate_dir.mkdir(exist_ok=True)

    def Load(self):
        pass


class BuildType(Enum):
    STATIC_LIB = ".a"
    SHARED_LIB = ".so"
    EXECUTABLE = "" if os.name != "nt" else ".exe"


class KubenetesMeta(Meta):

    def Load(self):
        pass


class DockerMeta(Meta):

    def Load(self):
        pass


class MetaLoader:

    def __init__(self):
        self.working_dir = Path(os.curdir)
    
    def LoadMetas(self) -> Dict[str, Meta]:
        metas = {}

        for meta_file in self.working_dir.glob("**/*.Build.py"):
            meta = Meta.LoadMeta(meta_file)
            if meta:
                metas[os.path.basename(meta_file)] = meta
        
        print(metas)

        return metas


