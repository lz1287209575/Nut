#pragma once

#include "Config/NConfig.h"
#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Containers/CArray.h"
#include "Containers/CHashMap.h"
#include "Threading/CThread.h"
#include "Delegates/CDelegate.h"
#include "FileSystem/NFileSystem.h"

namespace NLib
{

/**
 * @brief 配置层级信息
 */
struct LIBNLIB_API NConfigLayer
{
    CString Name;                                    // 层级名称
    int32_t Priority;                               // 优先级（数值越大优先级越高）
    CConfigValue Config;                            // 配置数据
    bool bReadOnly;                                 // 是否只读
    CString FilePath;                               // 关联的文件路径（如果有）
    
    NConfigLayer()
        : Priority(0), bReadOnly(false)
    {}
    
    NConfigLayer(const CString& InName, int32_t InPriority = 0, bool InReadOnly = false)
        : Name(InName), Priority(InPriority), bReadOnly(InReadOnly)
    {}
};

/**
 * @brief 配置管理器 - 管理分层配置系统
 */
class LIBNLIB_API NConfigManager : public CObject
{
    NCLASS()
class NConfigManager : public NObject
{
    GENERATED_BODY()

public:
    // 配置变化回调
    using ConfigChangeCallback = CDelegate<void(const CString&, const CConfigValue&, const CConfigValue&)>;
    
    NConfigManager();
    virtual ~NConfigManager();
    
    // 单例访问
    static NConfigManager& GetInstance();
    
    // 初始化和清理
    void Initialize();
    void Shutdown();
    
    // 配置层级管理
    void AddLayer(const CString& LayerName, int32_t Priority = 0, bool bReadOnly = false);
    void RemoveLayer(const CString& LayerName);
    bool HasLayer(const CString& LayerName) const;
    CArray<CString> GetLayerNames() const;
    void SetLayerPriority(const CString& LayerName, int32_t Priority);
    int32_t GetLayerPriority(const CString& LayerName) const;
    void SetLayerReadOnly(const CString& LayerName, bool bReadOnly);
    bool IsLayerReadOnly(const CString& LayerName) const;
    
    // 配置文件加载
    bool LoadConfig(const CString& FilePath);
    bool LoadConfigToLayer(const CString& FilePath, const CString& LayerName);
    bool SaveConfig(const CString& FilePath) const;
    bool SaveLayerConfig(const CString& LayerName, const CString& FilePath) const;
    bool ReloadConfig(const CString& FilePath);
    bool ReloadAllConfigs();
    
    // 从字符串加载配置
    bool LoadConfigFromString(const CString& ConfigString, const CString& Format = "json");
    bool LoadConfigToLayerFromString(const CString& ConfigString, const CString& LayerName, const CString& Format = "json");
    
    // 配置访问 - 获取合并后的配置值
    const CConfigValue& GetConfig(const CString& KeyPath) const;
    CConfigValue& GetConfig(const CString& KeyPath);
    
    // 类型安全的配置访问
    template<typename TType>
    TType GetValue(const CString& KeyPath, const TType& DefaultValue = TType{}) const;
    
    bool GetBool(const CString& KeyPath, bool DefaultValue = false) const;
    int32_t GetInt(const CString& KeyPath, int32_t DefaultValue = 0) const;
    int64_t GetInt64(const CString& KeyPath, int64_t DefaultValue = 0) const;
    float GetFloat(const CString& KeyPath, float DefaultValue = 0.0f) const;
    double GetDouble(const CString& KeyPath, double DefaultValue = 0.0) const;
    CString GetString(const CString& KeyPath, const CString& DefaultValue = "") const;
    
    // 配置设置 - 设置到最高优先级的可写层
    void SetValue(const CString& KeyPath, const CConfigValue& Value);
    void SetValue(const CString& KeyPath, bool Value);
    void SetValue(const CString& KeyPath, int32_t Value);
    void SetValue(const CString& KeyPath, int64_t Value);
    void SetValue(const CString& KeyPath, float Value);
    void SetValue(const CString& KeyPath, double Value);
    void SetValue(const CString& KeyPath, const CString& Value);
    
    // 层级特定的配置设置
    void SetValueInLayer(const CString& LayerName, const CString& KeyPath, const CConfigValue& Value);
    const CConfigValue& GetLayerConfig(const CString& LayerName, const CString& KeyPath) const;
    
    // 配置查询
    bool HasConfig(const CString& KeyPath) const;
    bool HasConfigInLayer(const CString& LayerName, const CString& KeyPath) const;
    void RemoveConfig(const CString& KeyPath);
    void RemoveConfigFromLayer(const CString& LayerName, const CString& KeyPath);
    
    // 配置监听
    void WatchConfig(const CString& KeyPath, ConfigChangeCallback Callback);
    void UnwatchConfig(const CString& KeyPath);
    void UnwatchAllConfig();
    
    // 热重载支持
    void EnableHotReload(bool bEnable = true);
    bool IsHotReloadEnabled() const;
    void SetHotReloadInterval(int32_t IntervalMs);
    int32_t GetHotReloadInterval() const;
    void CheckForConfigChanges();
    
    // 配置加载器管理
    void RegisterLoader(TSharedPtr<IConfigLoader> Loader);
    void UnregisterLoader(const CString& Extensions);
    TSharedPtr<IConfigLoader> FindLoader(const CString& FilePath) const;
    CArray<CString> GetSupportedExtensions() const;
    
    // 配置导出
    CString ExportToJson(bool Pretty = true) const;
    CString ExportLayerToJson(const CString& LayerName, bool Pretty = true) const;
    void ExportToFile(const CString& FilePath) const;
    
    // 配置合并
    CConfigValue GetMergedConfig() const;
    void MergeConfig(const CConfigValue& Config, const CString& TargetLayer = "");
    
    // 配置验证
    bool ValidateConfig() const;
    void SetConfigSchema(const CConfigValue& Schema);
    const CConfigValue& GetConfigSchema() const;
    
    // 命令行参数集成
    void ParseCommandLineArgs(int argc, char* argv[]);
    void ParseCommandLineArgs(const CArray<CString>& Args);
    void SetCommandLineLayer(const CString& LayerName);
    
    // 环境变量集成
    void LoadEnvironmentVariables(const CString& Prefix = "NLIB_");
    void SetEnvironmentLayer(const CString& LayerName);
    
    // 统计信息
    int32_t GetTotalConfigCount() const;
    int32_t GetLayerConfigCount(const CString& LayerName) const;
    CHashMap<CString, int32_t> GetConfigStatistics() const;
    
    // 调试支持
    void DumpConfig() const;
    void DumpLayer(const CString& LayerName) const;
    CString GetConfigReport() const;

private:
    // 内部数据结构
    CArray<NConfigLayer> ConfigLayers;
    CHashMap<CString, TSharedPtr<IConfigLoader>> ConfigLoaders;
    TSharedPtr<NConfigWatcher> ConfigWatcher;
    CConfigValue ConfigSchema;
    
    // 热重载相关
    bool bHotReloadEnabled;
    int32_t HotReloadIntervalMs;
    CHashMap<CString, int64_t> FileModificationTimes;
    TSharedPtr<class CThread> HotReloadThread;
    CAtomic<bool> bStopHotReload;
    
    // 层级映射
    CHashMap<CString, int32_t> LayerNameToIndex;
    
    // 命令行和环境变量层
    CString CommandLineLayerName;
    CString EnvironmentLayerName;
    
    // 线程安全
    mutable NMutex ConfigMutex;
    
    // 内部方法
    void RebuildLayerIndex();
    void SortLayersByPriority();
    NConfigLayer* FindLayer(const CString& LayerName);
    const NConfigLayer* FindLayer(const CString& LayerName) const;
    NConfigLayer* FindWritableLayer();
    
    CConfigValue MergeLayerConfigs() const;
    void NotifyConfigChange(const CString& KeyPath, const CConfigValue& OldValue, const CConfigValue& NewValue);
    
    // 热重载实现
    void HotReloadThreadMain();
    bool HasFileChanged(const CString& FilePath) const;
    void UpdateFileModificationTime(const CString& FilePath);
    
    // 命令行解析
    bool ParseCommandLineArg(const CString& Arg, CString& OutKey, CString& OutValue) const;
    CConfigValue ParseCommandLineValue(const CString& Value) const;
    
    // 环境变量解析
    CString GetEnvironmentVariable(const CString& Name) const;
    
    // 配置验证
    bool ValidateConfigValue(const CConfigValue& Value, const CConfigValue& Schema) const;
    
    // 默认加载器注册
    void RegisterDefaultLoaders();
    
    // 禁止复制
    NConfigManager(const NConfigManager&) = delete;
    NConfigManager& operator=(const NConfigManager&) = delete;
};

// 模板实现
template<typename TType>
TType NConfigManager::GetValue(const CString& KeyPath, const TType& DefaultValue) const
{
    const CConfigValue& Config = GetConfig(KeyPath);
    
    if constexpr (std::is_same_v<TType, bool>)
    {
        return Config.AsBool(DefaultValue);
    }
    else if constexpr (std::is_same_v<TType, int32_t>)
    {
        return Config.AsInt(DefaultValue);
    }
    else if constexpr (std::is_same_v<TType, int64_t>)
    {
        return Config.AsInt64(DefaultValue);
    }
    else if constexpr (std::is_same_v<TType, float>)
    {
        return Config.AsFloat(DefaultValue);
    }
    else if constexpr (std::is_same_v<TType, double>)
    {
        return Config.AsDouble(DefaultValue);
    }
    else if constexpr (std::is_same_v<TType, CString>)
    {
        return Config.AsString(DefaultValue);
    }
    else
    {
        static_assert(false, "Unsupported type for GetValue");
        return DefaultValue;
    }
}

/**
 * @brief 配置管理器便捷宏
 */
#define CONFIG_GET_BOOL(KeyPath, Default) NLib::NConfigManager::GetInstance().GetBool(KeyPath, Default)
#define CONFIG_GET_INT(KeyPath, Default) NLib::NConfigManager::GetInstance().GetInt(KeyPath, Default)
#define CONFIG_GET_FLOAT(KeyPath, Default) NLib::NConfigManager::GetInstance().GetFloat(KeyPath, Default)
#define CONFIG_GET_STRING(KeyPath, Default) NLib::NConfigManager::GetInstance().GetString(KeyPath, Default)

#define CONFIG_SET_BOOL(KeyPath, Value) NLib::NConfigManager::GetInstance().SetValue(KeyPath, Value)
#define CONFIG_SET_INT(KeyPath, Value) NLib::NConfigManager::GetInstance().SetValue(KeyPath, Value)
#define CONFIG_SET_FLOAT(KeyPath, Value) NLib::NConfigManager::GetInstance().SetValue(KeyPath, Value)
#define CONFIG_SET_STRING(KeyPath, Value) NLib::NConfigManager::GetInstance().SetValue(KeyPath, Value)

#define CONFIG_HAS(KeyPath) NLib::NConfigManager::GetInstance().HasConfig(KeyPath)
#define CONFIG_WATCH(KeyPath, Callback) NLib::NConfigManager::GetInstance().WatchConfig(KeyPath, Callback)

} // namespace NLib
