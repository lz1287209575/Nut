# LibNut

LibNut 是 Nut 项目的通用基础库，封装了内存分配（jemalloc）和日志（spdlog）功能，供所有微服务和主程序复用。

## 目录结构

```
LibNut/
├── include/           # 头文件（NutAllocator.h, NutLogger.h）
├── src/               # 实现文件（NutAllocator.cpp, NutLogger.cpp）
├── Meta/              # 构建元数据（LibNut.Build.py）
├── README.md          # 说明文档
```

## 依赖
- jemalloc（通过 ThirdParty 子模块引入）
- spdlog（通过 ThirdParty 子模块引入）

## 用法
- 通过 NutAllocator/NutLogger 统一使用内存分配和日志功能
- 适配 Nut 工程自定义构建系统 