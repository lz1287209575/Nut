ServiceMeta = {
    "name": "ServiceAllocateTest",
    "sources": [
        "../ServiceAllocateTest.cpp",
        "../../Sources/ServiceAllocateManager.cpp",
        "../../Sources/ConfigManager.cpp",
        "../../Sources/Logger.cpp"
    ],
    "include_dirs": [
        "../../Sources",
        ".."
    ],
    "config_files": [
        "../../Configs/ServiceAllocateConfig.json"
    ],
    "dependencies": [
        "protobuf",
        "threads"
    ],
    "output": "ServiceAllocateTest"
} 