DockerMeta = {
    "base_image": "ubuntu:22.04",
    "copy_files": [
        "../Sources/PlayerServiceMain",
        "../Configs/PlayerServiceConfig.json"
    ],
    "expose_ports": [None],
    "entrypoint": ["./PlayerServiceMain", "--config", "PlayerServiceConfig.json"]
}
