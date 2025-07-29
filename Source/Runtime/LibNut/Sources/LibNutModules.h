#pragma once

// LibNut模块索引文件
// 定义各个模块的头文件路径，避免硬编码路径依赖

// ===============================
// 核心模块 (Core)
// ===============================
#define LIBNUT_CORE_NOBJECT_H "Core/NObject.h"

// ===============================
// 内存管理模块 (Memory)
// ===============================
#define LIBNUT_MEMORY_ALLOCATOR_H "Memory/NAllocator.h"
#define LIBNUT_MEMORY_MANAGER_H "Memory/NMemoryManager.h"
#define LIBNUT_MEMORY_GC_H "Memory/NGarbageCollector.h"

// ===============================
// 日志模块 (Logging)
// ===============================
#define LIBNUT_LOGGING_LOGGER_H "Logging/NLogger.h"

// ===============================
// 反射模块 (Reflection)
// ===============================
#define LIBNUT_REFLECTION_OBJECT_H "Reflection/NObjectReflection.h"

// ===============================
// 容器模块 (Containers)
// ===============================
#define LIBNUT_CONTAINERS_BASE_H "Containers/NContainer.h"
#define LIBNUT_CONTAINERS_STRING_H "Containers/NString.h"
#define LIBNUT_CONTAINERS_ARRAY_H "Containers/NArray.h"
#define LIBNUT_CONTAINERS_HASHMAP_H "Containers/NHashMap.h"
#define LIBNUT_CONTAINERS_LIB_H "Containers/NLib.h"

// ===============================
// 模块依赖关系图
// ===============================
/*
依赖层次结构（从底层到高层）：

Level 0 - 基础工具:
  - Logging/NLogger.h (最底层，无依赖)

Level 1 - 内存系统:
  - Memory/NAllocator.h (依赖: Logging)
  - Memory/NMemoryManager.h (依赖: Logging, NAllocator)

Level 2 - 核心对象:
  - Core/NObject.h (依赖: Memory/NAllocator)
  - Memory/NGarbageCollector.h (依赖: NObject, NMemoryManager, Logging)

Level 3 - 反射系统:
  - Reflection/NObjectReflection.h (依赖: Core/NObject)

Level 4 - 容器系统:
  - Containers/NContainer.h (依赖: Core/NObject, Memory/*)
  - Containers/NString.h (依赖: NContainer, Logging)
  - Containers/NArray.h (依赖: NContainer, Logging)
  - Containers/NHashMap.h (依赖: NContainer, Logging)

Level 5 - 高级功能:
  - Containers/NLib.h (依赖: 所有容器)
  - LibNut.h (主入口，依赖所有模块)
*/

// ===============================
// 前向声明帮助宏
// ===============================
#define LIBNUT_FORWARD_DECLARE_CORE() \
    namespace Nut { \
        class NObject; \
    }

#define LIBNUT_FORWARD_DECLARE_MEMORY() \
    namespace Nut { \
        class NAllocator; \
        class NMemoryManager; \
        class NGarbageCollector; \
    }

#define LIBNUT_FORWARD_DECLARE_LOGGING() \
    namespace Nut { \
        class NLogger; \
    }

#define LIBNUT_FORWARD_DECLARE_CONTAINERS() \
    class NString; \
    template<typename T> class NArray; \
    template<typename K, typename V> class NHashMap; \
    class NContainer;

// ===============================
// 模块功能开关 (编译时配置)
// ===============================
#ifndef LIBNUT_DISABLE_LOGGING
    #define LIBNUT_ENABLE_LOGGING 1
#endif

#ifndef LIBNUT_DISABLE_REFLECTION
    #define LIBNUT_ENABLE_REFLECTION 1
#endif

#ifndef LIBNUT_DISABLE_CONTAINERS
    #define LIBNUT_ENABLE_CONTAINERS 1
#endif

#ifndef LIBNUT_DISABLE_MEMORY_TRACKING
    #define LIBNUT_ENABLE_MEMORY_TRACKING 1
#endif

// ===============================
// 平台特定包含
// ===============================
#ifdef LIBNUT_PLATFORM_WINDOWS
    // Windows特定的模块配置
#elif defined(LIBNUT_PLATFORM_MACOS)
    // macOS特定的模块配置
#elif defined(LIBNUT_PLATFORM_LINUX)
    // Linux特定的模块配置
#endif