from pathlib import Path

from Core.MetaLoader import BuildMeta, BuildType


class LibNutMeta(BuildMeta):
    
    def __init__(self, name: str, meta_file: Path):
        super().__init__(name, meta_file)    

        self.build_type = BuildType.STATIC_LIB