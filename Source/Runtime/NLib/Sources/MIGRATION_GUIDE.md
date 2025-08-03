# NLib 反射系统迁移指南

## 概述

本文档指导您将现有的 `DECLARE_NOBJECT_CLASS` 系统迁移到新的 `NCLASS()` + `GENERATED_BODY()` 系统。

## 迁移步骤

### 1. 头文件修改

**旧系统：**
```cpp
class NMyClass : public NObject
{
    DECLARE_NOBJECT_CLASS(NMyClass, NObject)
public:
    // 类实现...
};
```

**新系统：**
```cpp
NCLASS()
class NMyClass : public NObject
{
    GENERATED_BODY()
public:
    // 类实现...
};
```

### 2. 源文件修改

**旧系统：**
```cpp
#include "NMyClass.h"
// 无需额外包含
```

**新系统：**
```cpp
#include "NMyClass.h"
#include "NMyClass.generate.h"  // 必须包含生成的头文件
```

### 3. 属性反射

**旧系统：**
```cpp
class NMyClass : public NObject
{
    DECLARE_NOBJECT_CLASS(NMyClass, NObject)
public:
    int32_t MyProperty;  // 无反射支持
};
```

**新系统：**
```cpp
NCLASS()
class NMyClass : public NObject
{
    GENERATED_BODY()
public:
    NPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gameplay")
    int32_t MyProperty;  // 支持反射
};
```

### 4. 函数反射

**旧系统：**
```cpp
class NMyClass : public NObject
{
    DECLARE_NOBJECT_CLASS(NMyClass, NObject)
public:
    void MyFunction() {}  // 无反射支持
};
```

**新系统：**
```cpp
NCLASS()
class NMyClass : public NObject
{
    GENERATED_BODY()
public:
    NFUNCTION(BlueprintCallable, Category="Utility")
    void MyFunction() {}  // 支持反射
};
```

## 迁移检查清单

### 必须完成的步骤：

- [ ] 将 `DECLARE_NOBJECT_CLASS` 替换为 `NCLASS()` + `GENERATED_BODY()`
- [ ] 在 `.cpp` 文件中包含对应的 `.generate.h` 文件
- [ ] 确保类名以 `N` 开头（NObject子类）
- [ ] 移除旧的类型信息相关代码

### 可选步骤：

- [ ] 添加 `NPROPERTY` 标记到需要反射的属性
- [ ] 添加 `NFUNCTION` 标记到需要反射的函数
- [ ] 添加元数据到 `NCLASS` 宏

## 常见问题

### Q: 为什么需要包含 .generate.h 文件？

A: `.generate.h` 文件包含由代码生成工具自动生成的反射代码，包括：
- 静态类型名称函数
- 类反射信息
- 属性和函数的反射元数据

### Q: 如果 .generate.h 文件不存在怎么办？

A: 需要运行代码生成工具来生成这些文件。工具会扫描所有使用 `NCLASS` 的类并生成相应的反射代码。

### Q: 可以混合使用新旧系统吗？

A: 技术上可以，但不推荐。建议统一使用新系统以获得更好的反射支持和代码一致性。

### Q: 迁移后编译错误怎么办？

A: 常见原因：
1. 忘记包含 `.generate.h` 文件
2. 类名不符合命名规范
3. 缺少必要的头文件包含

## 迁移工具

可以使用以下命令批量替换：

```bash
# 替换 DECLARE_NOBJECT_CLASS
find . -name "*.h" -exec sed -i 's/DECLARE_NOBJECT_CLASS(\([^,]*\), \([^)]*\))/NCLASS()\nclass \1 : public \2\n{\n    GENERATED_BODY()/g' {} \;

# 添加 .generate.h 包含（需要手动检查）
find . -name "*.cpp" -exec grep -l "NCLASS" {} \; | xargs -I {} echo "#include \"{}.generate.h\"" >> {}
```

## 示例迁移

### 完整示例

**迁移前：**
```cpp
// NMyClass.h
class NMyClass : public NObject
{
    DECLARE_NOBJECT_CLASS(NMyClass, NObject)
public:
    int32_t Value;
    void DoSomething();
};
```

```cpp
// NMyClass.cpp
#include "NMyClass.h"
// 实现...
```

**迁移后：**
```cpp
// NMyClass.h
NCLASS()
class NMyClass : public NObject
{
    GENERATED_BODY()
public:
    NPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gameplay")
    int32_t Value;
    
    NFUNCTION(BlueprintCallable, Category="Utility")
    void DoSomething();
};
```

```cpp
// NMyClass.cpp
#include "NMyClass.h"
#include "NMyClass.generate.h"
// 实现...
```

## 注意事项

1. **命名规范**：确保所有NObject子类都以 `N` 开头
2. **包含顺序**：`.generate.h` 文件应该在类定义之后包含
3. **编译依赖**：新系统需要代码生成工具的支持
4. **向后兼容**：旧系统暂时保留以支持渐进式迁移

## 下一步

完成迁移后，您可以：
1. 利用完整的反射功能
2. 使用脚本绑定系统
3. 享受更好的IDE支持
4. 使用序列化功能 