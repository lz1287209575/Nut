# NLib 命名规范

## 概述
NLib采用统一的大驼峰命名规范（PascalCase），并通过前缀来区分不同类型的代码实体。

## 命名规则

### 通用规则
- **所有变量、函数、类都采用大驼峰命名**
- **文件名不包含类型前缀**

### 类型前缀规范

#### 类 (Classes)
- **NObject派生类**: `N***` (例: `NPlayer`, `NGameInstance`, `NNetworkManager`)
- **普通类**: `C***` (例: `CLogger`, `NFileSystem`, `CConfig`)
- **接口**: `I***` (例: `ISerializable`, `INetworkHandler`, `IRenderable`)
- **结构体**: `S***` (例: `SPlayerData`, `SGameConfig`, `SNetworkPacket`)
- **模板类**: `T***` (例: `TArray`, `TMap`, `TSharedPtr`)

#### 函数命名
- **所有函数**: 大驼峰 (例: `InitializeNetwork()`, `GetPlayerCount()`, `SendMessage()`)

#### 变量命名
- **所有变量**: 大驼峰 (例: `PlayerCount`, `NetworkSocket`, `ConfigPath`)
- **成员变量**: 大驼峰 (例: `PlayerCount`, `NetworkSocket`)
- **局部变量**: 大驼峰 (例: `SocketHandle`, `ConfigPath`)

#### 常量命名
- **宏常量**: 大写下划线 (例: `MAX_PLAYERS`, `DEFAULT_PORT`)
- **const/constexpr**: 'k' + 大驼峰 (例: `kMaxPlayers`, `kDefaultPort`)

#### 模板参数
- **具体语义化命名**: 
  - `TType` - 类型参数
  - `TArgs` - 参数包
  - `TElement` - 元素类型
  - `TKey`, `TValue` - 键值对类型
  - `TSize` - 大小类型

### 文件组织规范

#### 模块化目录结构
- **文件必须按模块名字分属在不同文件夹**
- **每个模块包含相关的类和功能**

```
Sources/
├── Core/           # 核心功能 (NObject, 基础宏)
├── Memory/         # 内存管理 (GC, 智能指针, 分配器)
├── Containers/     # 容器类 (TArray, TMap, TSet等)
├── Reflection/     # 反射系统 (类型信息, 属性访问)
├── String/         # 字符串处理 (CString, 字符编码)
├── Math/           # 数学库 (向量, 矩阵, 四元数)
├── IO/             # 文件系统 (文件读写, 路径处理)
├── Threading/      # 线程和并发 (线程池, 锁, 原子操作)
├── Network/        # 网络功能 (Socket, HTTP, 协议)
├── Serialization/  # 序列化 (JSON, 二进制, XML)
├── Logging/        # 日志系统 (日志级别, 输出格式)
├── Config/         # 配置管理 (配置文件解析)
├── Events/         # 事件系统 (事件分发, 委托)
├── Time/           # 时间处理 (计时器, 时间戳)
└── Macros/         # 宏定义 (反射宏, 工具宏)
```

#### 文件命名
- **头文件**: `ClassName.h` (不包含前缀，例: `Object.h`, `NetworkManager.h`)
- **源文件**: `ClassName.cpp` (例: `Object.cpp`, `NetworkManager.cpp`)
- **文件放置**: 根据功能放入对应模块文件夹

#### 反射类规范
- **一个文件只能包含一个NCLASS**
- **NCLASS装饰的类必须在include最后包含生成的头文件**
```cpp
#include "Core/Object.h"
// 其他includes...
#include "Player.generate.h"  // 必须在最后一行
```

### NObject系统

#### 基础对象
- **所有对象继承自NObject**: 提供统一的内存管理、GC等功能
- **NObject派生类命名**: 必须以'N'开头

#### 反射宏系统
- **NCLASS()**: 标记类需要反射支持
- **NPROPERTY()**: 标记属性需要反射
- **NFUNCTION()**: 标记函数需要反射
- **NMETHOD()**: 标记方法需要反射
- **GENERATED_BODY()**: 插入生成的反射代码

### 命名空间
- **主命名空间**: `NLib`
- **子命名空间**: 大驼峰 (例: `NLib::Memory`, `NLib::Network`, `NLib::Reflection`)

## 示例

### 反射类示例
```cpp
// Player.h
#pragma once
#include "Object.h"

namespace NLib
{
    NCLASS()
    class NPlayer : public NObject
    {
        GENERATED_BODY()
        
    public:
        NPROPERTY()
        CString PlayerName;
        
        NPROPERTY()
        int32_t PlayerLevel;
        
        NFUNCTION()
        void SetPlayerName(const CString& Name);
        
        NMETHOD()
        int32_t GetPlayerLevel() const;
    };
}

#include "Player.generate.h"
```

### 普通类示例
```cpp
// Logger.h
#pragma once

namespace NLib
{
    class CLogger
    {
    public:
        static void Log(const CString& Message);
        static void Error(const CString& Message);
    };
}
```

### 模板类示例
```cpp
// Array.h
#pragma once

namespace NLib
{
    template<typename TElement>
    class TArray
    {
    public:
        void Add(const TElement& Element);
        TElement& Get(int32_t Index);
    };
}
```

## 注意事项
1. 严格遵循前缀规范，便于代码识别和维护
2. NCLASS装饰的类必须正确包含.generate.h文件
3. 模板参数使用语义化命名，提高代码可读性
4. 所有需要GC管理的对象必须继承自NObject