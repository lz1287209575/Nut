# NLib - Nut引擎核心库

NLib是Nut游戏引擎的核心库，提供对象系统、内存管理、容器、反射和日志功能。

## 🏗️ 模块架构

NLib采用模块化设计，各模块功能独立且层次清晰：

```
NLib/
├── 📁 Core/           # 核心对象系统
├── 📁 Memory/         # 内存管理系统
├── 📁 Logging/        # 日志系统
├── 📁 Reflection/     # 反射系统
├── 📁 Containers/     # 容器库
├── 📁 Testing/        # 测试和示例
├── 📄 NLib.h          # 主入口头文件  
├── 📄 NLib.cpp        # 主入口实现
└── 📄 NLibModules.h   # 模块索引和配置
```

## 📋 模块详情

### 🔧 Core - 核心对象系统
**文件**: `Core/NObject.h`, `Core/NObject.cpp`

提供所有Nut对象的基类，包含：
- 唯一对象ID管理
- 基础生命周期管理
- NCLASS反射宏系统
- 虚函数接口定义

**主要类**: `NObject`

### 🧠 Memory - 内存管理系统
**文件**: `Memory/NAllocator.h`, `Memory/NMemoryManager.h`, `Memory/NGarbageCollector.h`

提供高性能的内存管理：
- **NAllocator**: 内存分配器接口
- **NMemoryManager**: 统一内存管理器，支持内存池
- **NGarbageCollector**: 自动垃圾回收器

**特性**:
- 内存泄漏检测
- 分配追踪和统计
- 自动对象生命周期管理

### 📝 Logging - 日志系统
**文件**: `Logging/NLogger.h`, `Logging/NLogger.cpp`

基于spdlog的统一日志系统：
- 多级别日志 (Debug, Info, Warning, Error)
- 格式化输出支持
- 线程安全
- 可配置输出目标

**使用示例**:
```cpp
NLogger::GetLogger().Info("Hello {}", "World");
NLogger::GetLogger().Error("Error code: {}", 404);
```

### 🔍 Reflection - 反射系统
**文件**: `Reflection/NObjectReflection.h`, `Reflection/NObjectReflection.cpp`

运行时类型信息和反射功能：
- 类型注册和查询
- 属性和方法的运行时访问
- 动态对象创建
- 与NCLASS宏系统集成

### 📦 Containers - 容器库
**文件**: `Containers/NContainer.h`, `Containers/NString.h`, `Containers/NArray.h`, `Containers/NHashMap.h`

基于NObject的高性能STL容器：

#### NString - 字符串类
- Small String Optimization (SSO)
- UTF-8支持
- 丰富的字符串操作API
- 类型转换功能

#### NArray<T> - 动态数组
- 自动内存管理
- STL兼容接口
- GC集成
- 高性能实现

#### NHashMap<K,V> - 哈希表
- Robin Hood哈希算法
- 动态扩容
- 完整的映射接口
- 内存优化

### 🧪 Testing - 测试和示例
**文件**: `Testing/NExampleClass.h`, `Testing/NLibTest.cpp`

包含各种测试用例和使用示例。

## 🚀 快速开始

### 基本使用

```cpp
#include "LibNut.h"

int main() {
    // 初始化LibNut
    if (!LibNut::Initialize()) {
        return -1;
    }
    
    // 使用容器
    auto str = LibNut::MakeString("Hello World");
    auto arr = LibNut::MakeArray({1, 2, 3, 4, 5});
    auto map = LibNut::MakeHashMap({
        {LibNut::MakeString("key1"), 100},
        {LibNut::MakeString("key2"), 200}
    });
    
    // 日志输出
    LibNut::LogInfo("String: {}", str.CStr());
    LibNut::LogInfo("Array size: {}", arr.GetSize());
    LibNut::LogInfo("Map contains key1: {}", map.Contains(LibNut::MakeString("key1")));
    
    // 清理资源
    LibNut::Shutdown();
    return 0;
}
```

### 自定义对象

```cpp
#include "LibNut.h"

NCLASS()
class MyObject : public NObject {
    GENERATED_BODY()
    
public:
    NPROPERTY()
    NString name;
    
    NPROPERTY()
    int32_t value;
    
    NFUNCTION()
    void PrintInfo() {
        LibNut::LogInfo("MyObject: name={}, value={}", name.CStr(), value);
    }
    
    virtual NString ToString() const override {
        return NString::Format("MyObject(name='{}', value={})", name.CStr(), value);
    }
};
```

## 🔧 编译配置

### 编译器要求
- C++20或更高版本
- 支持的编译器：GCC 11+, Clang 13+, MSVC 2022

### 预处理器宏

| 宏名称 | 功能 | 默认值 |
|--------|------|--------|
| `LIBNUT_DEBUG` | 启用调试功能 | Debug版本启用 |
| `LIBNUT_ENABLE_PROFILING` | 启用性能分析 | 关闭 |
| `LIBNUT_AUTO_INITIALIZE` | 自动初始化/清理 | 关闭 |
| `LIBNUT_DISABLE_*` | 禁用特定模块 | 全部启用 |

### CMake集成

```cmake
# 添加LibNut源文件
file(GLOB_RECURSE LIBNUT_SOURCES "Source/Runtime/LibNut/Sources/*.cpp")
file(GLOB_RECURSE LIBNUT_HEADERS "Source/Runtime/LibNut/Sources/*.h")

# 创建库目标
add_library(LibNut STATIC ${LIBNUT_SOURCES} ${LIBNUT_HEADERS})

# 设置包含目录
target_include_directories(LibNut PUBLIC 
    "Source/Runtime/LibNut/Sources"
)

# 设置编译选项
target_compile_features(LibNut PUBLIC cxx_std_20)
target_compile_definitions(LibNut PUBLIC LIBNUT_VERSION="1.0.0")
```

## 🔗 依赖关系

### 外部依赖
- **spdlog**: 日志库
- **tcmalloc**: 内存分配器 (可选)

### 内部依赖层次
```
Level 0: Logging (无依赖)
Level 1: Memory (依赖 Logging)
Level 2: Core (依赖 Memory)
Level 3: Reflection (依赖 Core)
Level 4: Containers (依赖 Core, Memory, Logging)
Level 5: LibNut.h (统一入口)
```

## 📊 性能特性

- **内存效率**: 内存池分配，减少碎片
- **缓存友好**: 数据结构优化内存布局
- **零拷贝**: 移动语义和原地构造
- **模板特化**: 针对常用类型的优化

## 🛡️ 安全特性

- **内存安全**: 自动GC，防止内存泄漏
- **类型安全**: 强类型系统和编译时检查
- **线程安全**: 关键组件的并发保护
- **异常安全**: RAII和异常中性设计

## 📈 扩展性

LibNut设计为可扩展架构：
- 模块化设计，便于添加新功能
- 插件接口，支持自定义组件
- 配置系统，运行时调整行为
- 反射系统，支持脚本集成

## 🔍 调试和分析

### 内存分析
```cpp
// 获取内存统计
auto stats = LibNut::GetMemoryStats();
LibNut::LogInfo("Total allocated: {} bytes", stats.total_allocated);
LibNut::LogInfo("Peak usage: {} bytes", stats.peak_usage);
```

### 性能分析
```cpp
// 获取性能统计
auto perf = LibNut::GetPerformanceStats();
LibNut::LogInfo("Object creations: {}", perf.object_creations);
LibNut::LogInfo("Average GC time: {:.2f}ms", perf.average_gc_time_ms);
```

## 📄 许可证

LibNut是Nut引擎的一部分，遵循项目的许可证条款。