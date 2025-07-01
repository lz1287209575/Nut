# -*- coding: utf-8 -*-


from pathlib import Path

from Core.MetaLoader import Meta


class ServiceAllocateMeta(BuildMeta):
    
    def __init__(self, name: str, meta_file: Path):
        super().__init__(name, meta_file)
