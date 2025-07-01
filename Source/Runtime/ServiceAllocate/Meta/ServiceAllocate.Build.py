ServiceMeta = {
    "name": "ServiceAllocate",
    "sources": [
        "../Sources/ServiceAllocateMain.cpp",
        "../Sources/ServiceAllocateManager.cpp",
        "../Sources/ConfigManager.cpp",
        "../Sources/Logger.cpp"
    ],
    "include_dirs": [
        "../Sources"
    ],
    "proto_files": [
        "../Protos/serviceallocate.proto"
    ],
    "config_files": [
        "../Configs/ServiceAllocateConfig.json"
    ],
    "dependencies": [
        "protobuf",
        "threads"
    ],
    "output": "ServiceAllocateMain"
}
