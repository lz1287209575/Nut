LibNutMeta = {
    "name": "LibNut",
    "sources": [
        "../src/NutAllocator.cpp",
        "../src/NutLogger.cpp"
    ],
    "include_dirs": [
        "../include",
        "../../ThirdParty/spdlog/include",
        "../../ThirdParty/jemalloc/include"
    ],
    "dependencies": [
        "spdlog",
        "jemalloc"
    ],
    "output": "LibNut.a"
} 