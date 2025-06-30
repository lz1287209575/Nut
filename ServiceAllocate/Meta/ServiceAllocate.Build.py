ServiceMeta = {
    "name": "ServiceAllocate",
    "sources": [
        "../Sources/ServiceAllocateMain.cpp"
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
        "protobuf"
    ],
    "output": "ServiceAllocateMain"
}
