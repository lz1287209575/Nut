# ServiceAllocate 服务分配器

## 概述

ServiceAllocate 是一个高性能的微服务分配器，负责管理游戏服务器集群中的服务注册、发现、负载均衡和健康检查。

## 核心功能

### 🎯 服务注册与发现
- 支持动态服务注册和注销
- 按服务类型进行服务发现
- 自动维护服务索引

### ⚖️ 负载均衡
- 基于负载的服务选择策略
- 支持多种负载均衡算法
- 实时负载监控和更新

### 🏥 健康检查
- 自动健康状态监控
- 心跳超时检测
- 不健康服务自动隔离

### 📊 配置管理
- 灵活的配置文件支持
- 运行时配置更新
- 多级配置管理

## 架构设计

```
ServiceAllocate
├── ServiceAllocateManager    # 核心管理器
├── ConfigManager            # 配置管理
├── Logger                   # 日志系统
└── 测试程序                 # 功能验证
```

## 快速开始

### 1. 编译服务
```bash
# 在项目根目录执行
python BuildSystem/BuildScripts/BuildEngine.py build ServiceAllocate
```

### 2. 运行服务
```bash
# 运行主服务
./Build/ServiceAllocateMain

# 运行测试程序
./Build/ServiceAllocateTest
```

### 3. 配置说明

配置文件：`Configs/ServiceAllocateConfig.json`

```json
{
    "service_name": "ServiceAllocate",
    "listen_port": 50052,
    "log_level": "info",
    "health_check_interval": 30,
    "heartbeat_timeout": 60,
    "max_retries": 3,
    "max_services_per_type": 10,
    "load_balance_strategy": "least_load"
}
```

## API 使用示例

### 服务注册
```cpp
ServiceInfo serviceInfo;
serviceInfo.serviceName = "PlayerService-1";
serviceInfo.serviceType = "PlayerService";
serviceInfo.host = "192.168.1.100";
serviceInfo.port = 50051;
serviceInfo.currentLoad = 0;
serviceInfo.maxLoad = 100;
serviceInfo.isHealthy = true;

serviceManager->RegisterService(serviceInfo);
```

### 服务分配
```cpp
auto result = serviceManager->AllocateService("PlayerService", "Client-123");
if (result.success) {
    std::cout << "分配到服务: " << result.serviceName 
              << " (" << result.host << ":" << result.port << ")" << std::endl;
}
```

### 服务发现
```cpp
auto services = serviceManager->GetServicesByType("PlayerService");
for (const auto& service : services) {
    std::cout << "服务: " << service.serviceName 
              << " 负载: " << service.currentLoad << "/" << service.maxLoad << std::endl;
}
```

## 负载均衡策略

### 最少负载策略 (least_load)
- 选择当前负载最低的服务
- 适合大多数场景
- 自动避免过载服务

### 轮询策略 (round_robin)
- 按顺序分配服务
- 负载分布均匀
- 适合服务性能相近的场景

## 健康检查机制

### 心跳检测
- 服务定期发送心跳信号
- 超时自动标记为不健康
- 可配置心跳间隔和超时时间

### 自动恢复
- 服务重新发送心跳后自动恢复
- 支持手动健康状态更新
- 实时监控服务状态

## 性能特性

- **高并发支持**：线程安全设计，支持多线程并发访问
- **低延迟**：内存中的服务索引，快速查找和分配
- **高可用性**：自动故障检测和恢复
- **可扩展性**：支持动态添加和移除服务

## 监控和日志

### 日志级别
- `DEBUG`：详细调试信息
- `INFO`：一般信息
- `WARN`：警告信息
- `ERROR`：错误信息

### 关键指标
- 服务注册数量
- 服务分配成功率
- 平均响应时间
- 健康服务比例

## 部署建议

### 单机部署
- 适合小规模集群
- 简单配置和运维
- 单点故障风险

### 集群部署
- 多实例负载均衡
- 高可用性保证
- 需要额外的协调机制

## 故障排除

### 常见问题

1. **服务分配失败**
   - 检查目标服务类型是否存在
   - 确认服务健康状态
   - 验证负载是否超限

2. **心跳超时**
   - 检查网络连接
   - 调整心跳间隔配置
   - 确认服务正常运行

3. **性能问题**
   - 监控服务数量
   - 检查负载分布
   - 优化配置参数

## 扩展开发

### 添加新的负载均衡策略
1. 在 `ServiceAllocateManager` 中添加新的选择算法
2. 更新配置支持
3. 添加相应的测试用例

### 集成外部监控系统
1. 实现监控接口
2. 添加指标收集
3. 支持告警机制

## 贡献指南

1. 遵循项目的代码规范
2. 添加完整的测试用例
3. 更新相关文档
4. 提交 Pull Request

## 许可证

本项目遵循项目根目录的 LICENSE 文件。 