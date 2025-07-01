from curses import meta
import glob
import os
import platform
import subprocess
from pathlib import Path
import shutil

from Core.MetaLoader import Meta, BuildMeta


def BuildService(service_meta: Meta):
    service_name = service_meta.name
    print(f"==== 编译服务: {service_name} ====")
    project_root = Path(os.curdir)
    
    if not isinstance(service_meta, BuildMeta):
        raise ValueError(f"服务 {service_name} 不是 {BuildMeta.__name__} 类型")

    # 1. 生成Protobuf代码
    if service_meta.protos_dir.exists():
        for proto_rel in service_meta.protos_dir.glob("*.proto"):
            proto_file = proto_rel.resolve()
            if proto_file.exists():
                print(f"  [Proto] 生成 {proto_file.name} ...")
                protoc_cmd = [
                    FindProtoc(project_root),
                    f"--cpp_out={service_meta.sources_dir}",
                    f"--proto_path={service_meta.protos_dir}",
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
    cpp_files = list(service_meta.sources_dir.glob("*.cpp")) + list(service_meta.sources_dir.glob("*.c")) + list(service_meta.sources_dir.glob("*.cc"))
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
        obj_file = service_meta.intermediate_dir / (cpp_file.stem + obj_ext)
        compile_cmd = [
            "g++",
            "-std=c++20",
            "-I", str(service_meta.sources_dir),
            "-I", str(project_root / "Include"),
            "-c", str(cpp_file),
            "-o", str(obj_file)
        ]
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
    exe_name = service_meta.name
    exe_file = service_meta.output_dir / exe_name
    linker_cmd = ["g++"] + [str(f) for f in obj_files] + ["-o", str(exe_file)]
    print(f"  [链接] {exe_file.name}")
    try:
        result = subprocess.run(linker_cmd, check=True, capture_output=True, text=True)
        if result.stdout:
            print(result.stdout)
        if result.stderr:
            print(result.stderr)
    except subprocess.CalledProcessError as e:
        print(f"  [错误] 链接失败: {e.stderr}")
        return

    print(f"==== 服务 {service_name} 编译完成 ====")

def DetectCompiler():
    import platform
    import shutil
    import os
    sysname = platform.system()
    if sysname == "Windows":
        def find_cl():
            # 1. 先查 PATH
            cl_path = shutil.which("cl.exe")
            if cl_path:
                return cl_path
            # 2. 查常见 VS 安装目录
            vs_paths = [
                r"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC",
                r"C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC",
                r"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Tools\\MSVC",
                r"C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\Professional\\VC\\Tools\\MSVC",
                r"C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Tools\\MSVC",
                r"C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Tools\\MSVC",
                r"C:\\Program Files\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Tools\\MSVC",
                r"C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Tools\\MSVC",
            ]
            for base in vs_paths:
                if os.path.exists(base):
                    lst_version = os.listdir(base)
                    for v in sorted(lst_version, reverse=True): 
                        print(f"  [查找] {v}")
                        cl = os.path.join(base, v, "bin", "Hostx64", "x64", "cl.exe")
                        if os.path.exists(cl):
                            return cl
            # 3. 查注册表
            try:
                import winreg
                key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VS7")
                for i in range(0, winreg.QueryInfoKey(key)[1]):
                    val = winreg.EnumValue(key, i)[1]
                    cl = os.path.join(val, "VC", "Tools", "MSVC")
                    if os.path.exists(cl):
                        for v in os.listdir(cl):
                            cl_path = os.path.join(cl, v, "bin", "Hostx64", "x64", "cl.exe")
                            if os.path.exists(cl_path):
                                return cl_path
            except Exception:
                pass
            return None
        # 优先g++，再cl.exe
        for compiler in ["g++.exe", "C:/MinGW/bin/g++.exe"]:
            if shutil.which(compiler):
                return compiler, compiler, ".o"
        cl = find_cl()
        if cl:
            return cl, cl, ".obj"
    else:
        for compiler in ["g++", "clang++"]:
            if shutil.which(compiler):
                return compiler, compiler, ".o"
    return None, None, None

def CleanService(service_meta: Meta):
    service_name = service_meta.name
    output_dir = service_meta.output_dir
    intermediate_dir = service_meta.intermediate_dir
    if output_dir.exists():
        print(f"清理 {service_name} 的 Build 目录 ...")
        shutil.rmtree(output_dir)
    if intermediate_dir.exists():
        print(f"清理 {service_name} 的 Intermediate 目录 ...")
        shutil.rmtree(intermediate_dir)
    else:
        print(f"{service_name} 无 Build 目录，无需清理。")

def PackageService(service_meta, project_root):
    service_name = service_meta["name"]
    service_dir = FindServiceDir(service_meta, project_root)
    output_dir = service_dir / "Build"
    if not output_dir.exists():
        print(f"{service_name} 未编译，无需打包。")
        return
    import tarfile
    tar_path = service_dir / f"{service_name}.tar.gz"
    with tarfile.open(tar_path, "w:gz") as tar:
        for file in output_dir.iterdir():
            tar.add(file, arcname=file.name)
    print(f"已打包: {tar_path}")

def FindProtoc(project_root):
    local_protoc = project_root / "Tools" / "protoc" / "bin" / ("protoc.exe" if platform.system() == "Windows" else "protoc")
    if local_protoc.exists():
        return str(local_protoc)
    return shutil.which("protoc") or "protoc"

def GetClVersion(cl_path):
    """获取cl.exe主版本号和次版本号，返回(major, minor)，获取失败返回(None, None)"""
    import subprocess
    import re
    try:
        result = subprocess.run([cl_path], capture_output=True, text=True)
        # 典型输出: 'Microsoft (R) C/C++ Optimizing Compiler Version 19.29.30133 for x64'
        m = re.search(r"Version (\d+)\.(\d+)", result.stdout)
        if m:
            return int(m.group(1)), int(m.group(2))
    except Exception:
        pass
    return None, None 