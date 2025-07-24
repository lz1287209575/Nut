"""
UUID 生成器

为 XCode 和 Visual Studio 项目生成所需的 UUID
"""

import hashlib
import uuid
import platform
import subprocess
from typing import Optional


class UuidGenerator:
    """UUID 生成器"""
    
    def __init__(self, deterministic: bool = False):
        """
        初始化 UUID 生成器
        
        Args:
            deterministic: 是否使用确定性生成（基于内容哈希）
        """
        self.deterministic = deterministic
        self._counter = 0
    
    def generate(self, seed: Optional[str] = None) -> str:
        """
        生成 UUID
        
        Args:
            seed: 种子字符串，用于确定性生成
            
        Returns:
            24位大写十六进制 UUID（XCode 格式）
        """
        if self.deterministic and seed:
            return self._GenerateDeterministic(seed)
        else:
            return self._GenerateRandom()
    
    def GenerateGuid(self, name: str) -> str:
        """
        生成 GUID（Visual Studio 格式）
        
        Args:
            name: 用于生成 GUID 的名称
            
        Returns:
            标准 GUID 格式字符串
        """
        # 使用名称的 MD5 哈希生成确定性 GUID
        hash_bytes = hashlib.md5(name.encode('utf-8')).digest()
        
        # 转换为标准 GUID 格式
        guid_str = hash_bytes.hex()
        return "{{{}-{}-{}-{}-{}}}".format(
            guid_str[0:8],
            guid_str[8:12], 
            guid_str[12:16],
            guid_str[16:20],
            guid_str[20:32]
        ).upper()
    
    def _GenerateRandom(self) -> str:
        """生成随机 UUID"""
        if platform.system() == 'Darwin':
            # macOS 上使用 uuidgen
            try:
                result = subprocess.run(['uuidgen'], capture_output=True, text=True)
                if result.returncode == 0:
                    return result.stdout.strip().replace('-', '')[:24].upper()
            except:
                pass
        
        # fallback 到 Python uuid
        return str(uuid.uuid4()).replace('-', '')[:24].upper()
    
    def _GenerateDeterministic(self, seed: str) -> str:
        """基于种子生成确定性 UUID"""
        # 添加计数器确保唯一性
        full_seed = f"{seed}_{self._counter}"
        self._counter += 1
        
        # 使用 SHA-256 生成哈希
        hash_obj = hashlib.sha256(full_seed.encode('utf-8'))
        hash_hex = hash_obj.hexdigest()
        
        # 取前 24 位并转为大写
        return hash_hex[:24].upper()