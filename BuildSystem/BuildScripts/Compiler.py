import os
import platform
import subprocess
from pathlib import Path
import shutil

def BuildService(service_meta, project_root):
    service_name = service_meta["name"]
    print(f"==== 编译服务: {service_name} ====")
    service_dir = project_root / "MicroServices" / service_name
    sources_dir = service_dir / "Sources"
    protos_dir = service_dir / "Protos"
    output_dir = service_dir / "Build"
    output_dir.mkdir(exist_ok=True)

    # 1. 生成Protobuf代码
    if "proto_files" in service_meta and service_meta["proto_files"]:
        for proto_rel in service_meta["proto_files"]:
            proto_file = (protos_dir / Path(proto_rel).name).resolve()
            if proto_file.exists():
                print(f"  [Proto] 生成 {proto_file.name} ...")
                protoc_cmd = [
                    FindProtoc(project_root),
                    f"--cpp_out={sources_dir}",
                    f"--proto_path={protos_dir}",
                    str(proto_file)
                ]
                try:
                    subprocess.run(protoc_cmd, check=True)
                except Exception as e:
                    print(f"  [错误] Protobuf 生成失败: {e}")
                    return
            else:
                print(f"  [警告] 未找到proto文件: {proto_file}")

    # 2. 收集所有源文件
    cpp_files = list(sources_dir.glob("*.cpp"))
    if not cpp_files:
        print("  [错误] 未找到任何源文件！")
        return

    # 3. 检测编译器
    compiler, linker, obj_ext = DetectCompiler()
    if not compiler:
        print("  [错误] 未找到可用的C++编译器！")
        return

    # 4. 编译所有源文件
    obj_files = []
    for cpp_file in cpp_files:
        obj_file = output_dir / (cpp_file.stem + obj_ext)
        compile_cmd = [compiler, "-std=c++20", "-I", str(sources_dir), "-I", str(project_root / "Include"), "-c", str(cpp_file), "-o", str(obj_file)]
        print(f"  [编译] {cpp_file.name} -> {obj_file.name}")
        try:
            result = subprocess.run(compile_cmd, check=True, capture_output=True, text=True)
            if result.stdout:
                print(result.stdout)
            if result.stderr:
                print(result.stderr)
        except subprocess.CalledProcessError as e:
            print(f"  [错误] 编译失败: {e.stderr}")
            return
        obj_files.append(str(obj_file))

    # 5. 链接为可执行文件
    exe_name = service_meta.get("output", service_name)
    exe_file = output_dir / (exe_name + (".exe" if platform.system() == "Windows" else ""))
    link_cmd = [linker] + obj_files + ["-o", str(exe_file)]
    print(f"  [链接] {exe_file.name}")
    try:
        result = subprocess.run(link_cmd, check=True, capture_output=True, text=True)
        if result.stdout:
            print(result.stdout)
        if result.stderr:
            print(result.stderr)
    except subprocess.CalledProcessError as e:
        print(f"  [错误] 链接失败: {e.stderr}")
        return

    # 6. 拷贝配置和协议文件
    for config in service_meta.get("config_files", []):
        src = service_dir / "Configs" / Path(config).name
        if src.exists():
            shutil.copy(src, output_dir)
            print(f"  [拷贝] 配置文件: {src.name}")
    for proto in service_meta.get("proto_files", []):
        src = protos_dir / Path(proto).name
        if src.exists():
            shutil.copy(src, output_dir)
            print(f"  [拷贝] 协议文件: {src.name}")

    print(f"==== 服务 {service_name} 编译完成 ====")

def DetectCompiler():
    sysname = platform.system()
    if sysname == "Windows":
        # 优先g++，再cl.exe
        for compiler in ["g++.exe", "C:/MinGW/bin/g++.exe", "cl.exe"]:
            if shutil.which(compiler):
                if "cl.exe" in compiler:
                    return compiler, compiler, ".obj"
                else:
                    return compiler, compiler, ".o"
    else:
        for compiler in ["g++", "clang++"]:
            if shutil.which(compiler):
                return compiler, compiler, ".o"
    return None, None, None

def CleanService(service_meta, project_root):
    service_name = service_meta["name"]
    output_dir = project_root / "MicroServices" / service_name / "Build"
    if output_dir.exists():
        print(f"清理 {service_name} 的 Build 目录 ...")
        shutil.rmtree(output_dir)
    else:
        print(f"{service_name} 无 Build 目录，无需清理。")

def PackageService(service_meta, project_root):
    service_name = service_meta["name"]
    output_dir = project_root / "MicroServices" / service_name / "Build"
    if not output_dir.exists():
        print(f"{service_name} 未编译，无需打包。")
        return
    import tarfile
    tar_path = project_root / "MicroServices" / service_name / f"{service_name}.tar.gz"
    with tarfile.open(tar_path, "w:gz") as tar:
        for file in output_dir.iterdir():
            tar.add(file, arcname=file.name)
    print(f"已打包: {tar_path}")

def FindProtoc(project_root):
    local_protoc = project_root / "Tools" / "protoc" / "bin" / ("protoc.exe" if platform.system() == "Windows" else "protoc")
    if local_protoc.exists():
        return str(local_protoc)
    return shutil.which("protoc") or "protoc" 