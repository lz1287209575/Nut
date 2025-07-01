DockerMeta = {
    "base_image": "ubuntu:22.04",
    "copy_files": [
        "../Sources/ServiceAllocateMain",
        "../Configs/ServiceAllocateConfig.json"
    ],
    "expose_ports": [None],
    "entrypoint": ["./ServiceAllocateMain", "--config", "ServiceAllocateConfig.json"]
}
