#! /usr/bin/env python3
# -*- coding: utf-8 -*-


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
        if meta_file.name.endswith(".Build.py"):
            return BuildMeta(meta_file.stem.removesuffix(".Build").removesuffix(".py"), meta_file)
        elif meta_file.name.endswith(".Kubernetes.py"):
            return KubenetesMeta(meta_file.stem.removesuffix(".Kubernetes").removesuffix(".py"), meta_file)
        elif meta_file.name.endswith(".Docker.py"):
            return DockerMeta(meta_file.stem.removesuffix(".Docker").removesuffix(".py"), meta_file)
        else:
            raise ValueError(f"未找到 {meta_file.name} 对应的 Meta 类")

class BuildMeta(Meta):

    def __init__(self, name: str, meta_file: Path):
        super().__init__(name, meta_file)
        self.sources_dir = meta_file.parent.parent   / "Sources"
        self.protos_dir = meta_file.parent.parent / "Protos"
        self.output_dir = meta_file.parent.parent / "Build"
        self.intermediate_dir = meta_file.parent.parent / "Intermediate"

        self.output_dir.mkdir(exist_ok=True)
        self.intermediate_dir.mkdir(exist_ok=True)

    def Load(self):
        pass


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
            metas[os.path.basename(meta_file)] = Meta.LoadMeta(meta_file)

        return metas


