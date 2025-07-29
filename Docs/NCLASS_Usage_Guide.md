# NCLASS 系统使用指南

## 概述

NCLASS 系统是 Nut 引擎中类似于 Unreal Engine 的 UCLASS 系统的代码生成和反射框架。它允许你为 C++ 类添加元数据和反射支持，实现运行时类型信息、属性访问和函数调用。

## 核心组件

### 1. NutHeaderTools 代码生成器
- 位置：`Source/Programs/NutHeaderTools/`
- 功能：解析 C++ 头文件中的 NCLASS 宏，生成对应的 `.generate.h` 文件
- 技术：C# .NET 8.0，使用正则表达式解析

### 2. 反射系统
- 核心文件：`Source/Runtime/LibNut/Sources/NObjectReflection.h/cpp`
- 功能：提供运行时类型信息、对象创建、属性和函数反射

### 3. 宏定义系统
- 文件：`Source/Runtime/LibNut/Sources/NObject.h`
- 包含：`NCLASS`, `NPROPERTY`, `NFUNCTION`, `GENERATED_BODY` 等宏

## 基本使用方法

### 1. 声明一个 NCLASS

```cpp
#pragma once
#include "NObject.h"

NCLASS(meta = (Category="Gameplay", BlueprintType=true))
class MyGameObject : public NObject
{
    GENERATED_BODY()

public:
    MyGameObject();
    virtual ~MyGameObject();

    // 你的类实现...
};

// 包含生成的代码
#include "MyGameObject.generate.h"
```

### 2. 添加 NPROPERTY 属性

```cpp
NCLASS(meta = (Category="Gameplay"))
class MyPlayer : public NObject
{
    GENERATED_BODY()

public:
    // 基础属性
    NPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
    int32_t Health = 100;

    NPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats") 
    float Speed = 300.0f;

    NPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Info")
    std::string PlayerName = "Player";

private:
    // 私有成员不需要 NPROPERTY
    bool bIsAlive = true;
};
```

### 3. 添加 NFUNCTION 函数

```cpp
NCLASS(meta = (Category="Gameplay"))
class MyWeapon : public NObject
{
    GENERATED_BODY()

public:
    NFUNCTION(BlueprintCallable, Category="Combat")
    void Fire();

    NFUNCTION(BlueprintCallable, Category="Combat")
    int32_t GetAmmoCount() const;

    NFUNCTION(BlueprintCallable, Category="Combat")
    bool Reload(int32_t AmmoToAdd);

private:
    int32_t CurrentAmmo = 30;
};
```

## 构建流程

### 1. 手动生成头文件

```bash
# Unix 系统
./Tools/Scripts/GenerateHeaders.sh

# Windows 系统
Tools\Scripts\GenerateHeaders.bat
```

### 2. 集成到 IDE 项目生成

NutHeaderTools 会在以下时机自动运行：
- 运行 `GenerateProjectFiles.sh/bat` 时
- 构建系统检测到头文件变化时

### 3. 生成的文件

对于每个包含 `NCLASS` 的头文件 `MyClass.h`，会生成对应的 `MyClass.generate.h`：

```cpp
// MyClass.generate.h - 自动生成，请勿手动修改
#pragma once
#include "NObjectReflection.h"

// === MyClass 反射信息 ===
struct MyClassTypeInfo
{
    static constexpr const char* ClassName = "MyClass";
    static constexpr const char* BaseClassName = "NObject";
    static constexpr size_t PropertyCount = 3;
    static constexpr size_t FunctionCount = 2;
};

// MyClass 属性访问器
// Property: int32_t Health
// Property: float Speed
// Property: std::string PlayerName

// MyClass 函数调用器  
// Function: void Fire()
// Function: int32_t GetAmmoCount()

// 注册 MyClass 到反射系统
REGISTER_NCLASS_REFLECTION(MyClass);
```

## 反射系统 API

### 1. 获取类反射信息

```cpp
#include "NObjectReflection.h"

// 通过类名获取反射信息
const NClassReflection* reflection = 
    NObjectReflection::GetInstance().GetClassReflection("MyPlayer");

if (reflection) {
    std::cout << "类名: " << reflection->ClassName << std::endl;
    std::cout << "基类: " << reflection->BaseClassName << std::endl;
    std::cout << "属性数量: " << reflection->Properties.size() << std::endl;
}
```

### 2. 动态创建对象

```cpp
// 通过类名创建对象实例
NObject* obj = NObjectReflection::GetInstance().CreateInstance("MyPlayer");
if (obj) {
    MyPlayer* player = static_cast<MyPlayer*>(obj);
    // 使用 player...
}
```

### 3. 检查类继承关系

```cpp
bool isChild = NObjectReflection::GetInstance()
    .IsChildOf("MyPlayer", "NObject"); // true
```

### 4. 获取所有注册的类

```cpp
std::vector<std::string> allClasses = 
    NObjectReflection::GetInstance().GetAllClassNames();

for (const std::string& className : allClasses) {
    std::cout << "注册的类: " << className << std::endl;
}
```

## 最佳实践

### 1. 命名规范
- 类名使用大驼峰：`MyGameObject`
- 属性名使用大驼峰：`PlayerHealth`
- 函数名使用大驼峰：`GetPlayerHealth`

### 2. 元数据组织
```cpp
// 按功能分类
NCLASS(meta = (Category="Gameplay/Combat", BlueprintType=true))
class Weapon : public NObject { ... };

NCLASS(meta = (Category="Gameplay/Player", BlueprintType=true))  
class PlayerController : public NObject { ... };

NCLASS(meta = (Category="System/Network", BlueprintType=false))
class NetworkManager : public NObject { ... };
```

### 3. 属性分组
```cpp
NPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basic Stats")
int32_t Health;

NPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Basic Stats")
float Speed;

NPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Runtime Info")
std::string CurrentState;
```

### 4. 文件组织
```cpp
// MyClass.h
#pragma once
#include "NObject.h"

NCLASS(...)
class MyClass : public NObject
{
    GENERATED_BODY()
    // 类定义...
};

// 最后包含生成的头文件
#include "MyClass.generate.h"
```

## 故障排除

### 1. 生成的头文件不存在
- 确保运行了 `GenerateHeaders.sh/bat`
- 检查 NCLASS 宏语法是否正确
- 确保包含了 `GENERATED_BODY()` 宏

### 2. 编译错误
- 确保 `.generate.h` 文件在类定义之后 `#include`
- 检查 NObjectReflection.h 是否正确包含
- 确保 NutHeaderTools 构建成功

### 3. 反射信息不可用
- 检查 `REGISTER_NCLASS_REFLECTION` 宏是否被调用
- 确保对象在程序启动时被正确注册

### 4. 属性或函数未被识别
- 检查 NPROPERTY/NFUNCTION 宏语法
- 确保属性声明以分号结尾
- 检查函数声明是否完整

## 高级特性

### 1. 自定义元数据
```cpp
NCLASS(meta = (
    Category="Custom/AI", 
    BlueprintType=true,
    CustomMeta="value"
))
class AIController : public NObject { ... };
```

### 2. 继承链反射
系统自动处理继承关系，子类会继承父类的反射信息。

### 3. 模板类支持
目前模板类需要特殊处理，建议对具体的模板实例使用 NCLASS。

## 性能考虑

1. **编译时生成**：代码生成在编译时完成，运行时无额外开销
2. **反射查询**：反射信息存储在静态哈希表中，查询效率高
3. **内存占用**：每个类的反射信息占用少量静态内存

## 与 UE UCLASS 的对比

| 特性 | Nut NCLASS | UE UCLASS |
|------|------------|-----------|
| 代码生成 | C# 工具 | UHT (C++) |
| 反射系统 | 轻量级 | 功能完整 |
| 蓝图集成 | 计划中 | 完全支持 |
| 编辑器集成 | 计划中 | 完全支持 |
| 学习曲线 | 简单 | 中等 |

NCLASS 系统为 Nut 引擎提供了强大的反射和代码生成基础，为后续的蓝图系统、编辑器集成等高级功能奠定了基础。