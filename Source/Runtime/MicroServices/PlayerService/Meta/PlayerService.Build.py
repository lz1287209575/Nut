ServiceMeta = {
    "name": "PlayerService",
    "sources": [
        "../Sources/PlayerServiceMain.cpp"
    ],
    "include_dirs": [
        "../Sources"
    ],
    "proto_files": [
        "../Protos/playerservice.proto"
    ],
    "config_files": [
        "../Configs/PlayerServiceConfig.json"
    ],
    "dependencies": [
        "protobuf"
    ],
    "output": "PlayerServiceMain"
}
