#pragma once

/**
 * @file NBPEngine.h
 * @brief NLib Blueprint Script (NBP) 引擎实现
 * 
 * 自定义的可视化脚本语言，类似UE的Blueprint但基于文本
 * 专门为游戏服务器逻辑设计，语法简洁直观
 */

#include "Scripting/NScriptEngine.h"
#include "Threading/CThread.h"
#include "FileSystem/NFileSystem.h"
#include "Containers/CHashMap.h"
#include "Containers/CArray.h"

namespace NLib
{

/**
 * @brief NBP节点类型
 */
enum class ENBPNodeType : uint32_t
{
    // 基础节点
    Entry,              // 入口节点
    Exit,               // 出口节点
    Variable,           // 变量节点
    Constant,           // 常量节点
    
    // 控制流节点
    Sequence,           // 顺序执行
    Branch,             // 分支 (if)
    Switch,             // 切换 (switch)
    Loop,               // 循环
    ForEach,            // 遍历
    While,              // while循环
    DoWhile,            // do-while循环
    
    // 运算节点
    Add,                // 加法
    Subtract,           // 减法
    Multiply,           // 乘法
    Divide,             // 除法
    Modulo,             // 取模
    Power,              // 幂运算
    
    // 比较节点
    Equal,              // 等于
    NotEqual,           // 不等于
    Greater,            // 大于
    GreaterEqual,       // 大于等于
    Less,               // 小于
    LessEqual,          // 小于等于
    
    // 逻辑节点
    And,                // 逻辑与
    Or,                 // 逻辑或
    Not,                // 逻辑非
    
    // 函数节点
    FunctionCall,       // 函数调用
    FunctionDef,        // 函数定义
    Return,             // 返回
    
    // 对象节点
    CreateObject,       // 创建对象
    DestroyObject,      // 销毁对象
    GetProperty,        // 获取属性
    SetProperty,        // 设置属性
    CallMethod,         // 调用方法
    
    // 数组节点
    CreateArray,        // 创建数组
    ArrayGet,           // 数组获取
    ArraySet,           // 数组设置
    ArrayAdd,           // 数组添加
    ArrayRemove,        // 数组移除
    ArrayLength,        // 数组长度
    
    // 字符串节点
    StringConcat,       // 字符串连接
    StringLength,       // 字符串长度
    StringSubstring,    // 子字符串
    StringFormat,       // 字符串格式化
    
    // 事件节点
    Event,              // 事件节点
    Delay,              // 延迟节点
    Timer,              // 定时器节点
    
    // 网络节点
    SendMessage,        // 发送消息
    ReceiveMessage,     // 接收消息
    BroadcastMessage,   // 广播消息
    
    // 数据库节点
    QueryDatabase,      // 查询数据库
    UpdateDatabase,     // 更新数据库
    
    // 调试节点
    Print,              // 打印
    Log,                // 日志
    Assert,             // 断言
    Breakpoint,         // 断点
    
    // 自定义节点
    Custom              // 用户自定义节点
};

/**
 * @brief NBP数据类型
 */
enum class ENBPDataType : uint32_t
{
    Void,               // 空类型
    Boolean,            // 布尔
    Integer,            // 整数
    Float,              // 浮点数
    String,             // 字符串
    Object,             // 对象引用
    Array,              // 数组
    Map,                // 映射
    Function,           // 函数
    Event,              // 事件
    
    // 特殊类型
    Any,                // 任意类型
    Unknown             // 未知类型
};

/**
 * @brief NBP引脚方向
 */
enum class ENBPPinDirection : uint32_t
{
    Input,              // 输入引脚
    Output              // 输出引脚
};

/**
 * @brief NBP引脚类型
 */
enum class ENBPPinType : uint32_t
{
    Execution,          // 执行引脚 (白色)
    Data,               // 数据引脚 (根据类型着色)
    Event,              // 事件引脚 (红色)
    Delegate            // 委托引脚 (绿色)
};

/**
 * @brief NBP引脚定义
 */
struct LIBNLIB_API NBPPin
{
    CString Name;                   // 引脚名称
    ENBPPinDirection Direction;     // 方向
    ENBPPinType PinType;           // 引脚类型
    ENBPDataType DataType;         // 数据类型
    CString TypeName;              // 类型名称（用于对象类型）
    CScriptValue DefaultValue;     // 默认值
    bool bIsArray;                 // 是否数组
    bool bIsOptional;              // 是否可选
    CString Description;           // 描述
    
    NBPPin();
    NBPPin(const CString& InName, ENBPPinDirection InDirection, 
           ENBPPinType InPinType, ENBPDataType InDataType);
};

/**
 * @brief NBP连接定义
 */
struct LIBNLIB_API NBPConnection
{
    CString SourceNodeId;          // 源节点ID
    CString SourcePinName;         // 源引脚名称
    CString TargetNodeId;          // 目标节点ID
    CString TargetPinName;         // 目标引脚名称
    
    NBPConnection();
    NBPConnection(const CString& InSourceNodeId, const CString& InSourcePinName,
                  const CString& InTargetNodeId, const CString& InTargetPinName);
};

/**
 * @brief NBP节点定义
 */
class LIBNLIB_API NBPNode
{
public:
    NBPNode();
    NBPNode(const CString& InNodeId, ENBPNodeType InNodeType);
    virtual ~NBPNode();

    // 基本属性
    CString NodeId;                         // 节点唯一ID
    ENBPNodeType NodeType;                  // 节点类型
    CString Title;                          // 节点标题
    CString Category;                       // 节点分类
    CString Description;                    // 节点描述
    NVector2 Position;                      // 节点位置（用于可视化编辑器）
    NVector2 Size;                          // 节点大小
    
    // 引脚管理
    void AddInputPin(const NBPPin& Pin);
    void AddOutputPin(const NBPPin& Pin);
    void RemovePin(const CString& PinName);
    const NBPPin* GetPin(const CString& PinName) const;
    CArray<NBPPin> GetInputPins() const { return InputPins; }
    CArray<NBPPin> GetOutputPins() const { return OutputPins; }
    
    // 属性管理
    void SetProperty(const CString& Name, const CScriptValue& Value);
    CScriptValue GetProperty(const CString& Name) const;
    bool HasProperty(const CString& Name) const;
    
    // 执行接口
    virtual NScriptResult Execute(class CBPExecutionContext* Context) = 0;
    virtual bool ValidateNode() const;
    
    // 序列化
    virtual CString SerializeToText() const;
    virtual bool DeserializeFromText(const CString& Text);

protected:
    CArray<NBPPin> InputPins;
    CArray<NBPPin> OutputPins;
    CHashMap<CString, CScriptValue> Properties;
};

/**
 * @brief NBP图表（脚本文件）
 */
class LIBNLIB_API NBPGraph
{
public:
    NBPGraph();
    NBPGraph(const CString& InGraphName);
    virtual ~NBPGraph();

    // 基本属性
    CString GraphName;                      // 图表名称
    CString Description;                    // 图表描述
    CString Category;                       // 图表分类
    
    // 节点管理
    bool AddNode(TSharedPtr<NBPNode> Node);
    bool RemoveNode(const CString& NodeId);
    TSharedPtr<NBPNode> GetNode(const CString& NodeId) const;
    CArray<TSharedPtr<NBPNode>> GetAllNodes() const;
    CArray<TSharedPtr<NBPNode>> GetNodesByType(ENBPNodeType NodeType) const;
    
    // 连接管理
    bool AddConnection(const NBPConnection& Connection);
    bool RemoveConnection(const NBPConnection& Connection);
    CArray<NBPConnection> GetConnections() const { return Connections; }
    CArray<NBPConnection> GetNodeConnections(const CString& NodeId) const;
    
    // 变量管理
    void AddVariable(const CString& Name, ENBPDataType Type, const CScriptValue& DefaultValue = CScriptValue());
    void RemoveVariable(const CString& Name);
    void SetVariable(const CString& Name, const CScriptValue& Value);
    CScriptValue GetVariable(const CString& Name) const;
    
    // 函数管理
    void AddFunction(const CString& Name, const CArray<NBPPin>& Parameters, const CArray<NBPPin>& Returns);
    void RemoveFunction(const CString& Name);
    
    // 验证
    CArray<CString> ValidateGraph() const;
    bool IsValid() const;
    
    // 执行
    NScriptResult Execute(const CArray<CScriptValue>& Args = {});
    NScriptResult CallFunction(const CString& FunctionName, const CArray<CScriptValue>& Args = {});
    
    // 序列化
    CString SerializeToText() const;
    bool DeserializeFromText(const CString& Text);
    CString SerializeToJSON() const;
    bool DeserializeFromJSON(const CString& JSON);

private:
    CHashMap<CString, TSharedPtr<NBPNode>> Nodes;
    CArray<NBPConnection> Connections;
    CHashMap<CString, CScriptValue> Variables;
    CHashMap<CString, CArray<NBPPin>> Functions;
    
    // 执行辅助
    TSharedPtr<NBPNode> FindEntryNode() const;
    CArray<TSharedPtr<NBPNode>> GetExecutionPath(TSharedPtr<NBPNode> StartNode) const;
    
    mutable NMutex GraphMutex;
};

/**
 * @brief NBP执行上下文
 */
class LIBNLIB_API CBPExecutionContext
{
public:
    CBPExecutionContext(NBPGraph* InGraph);
    virtual ~CBPExecutionContext();

    // 执行控制
    void SetCurrentNode(TSharedPtr<NBPNode> Node) { CurrentNode = Node; }
    TSharedPtr<NBPNode> GetCurrentNode() const { return CurrentNode; }
    
    // 数据流
    void SetPinValue(const CString& NodeId, const CString& PinName, const CScriptValue& Value);
    CScriptValue GetPinValue(const CString& NodeId, const CString& PinName) const;
    
    // 变量访问
    void SetVariable(const CString& Name, const CScriptValue& Value);
    CScriptValue GetVariable(const CString& Name) const;
    
    // 函数调用
    NScriptResult CallFunction(const CString& FunctionName, const CArray<CScriptValue>& Args);
    
    // 执行控制
    void StopExecution() { bShouldStop = true; }
    bool ShouldStop() const { return bShouldStop; }
    
    // 调试支持
    void SetBreakpoint(const CString& NodeId);
    void RemoveBreakpoint(const CString& NodeId);
    bool IsBreakpoint(const CString& NodeId) const;
    
    // 错误处理
    void AddError(const CString& ErrorMessage);
    CArray<CString> GetErrors() const { return Errors; }

private:
    NBPGraph* Graph;
    TSharedPtr<NBPNode> CurrentNode;
    
    // 数据存储
    CHashMap<CString, CHashMap<CString, CScriptValue>> PinValues; // NodeId -> PinName -> Value
    CHashMap<CString, CScriptValue> LocalVariables;
    
    // 执行状态
    bool bShouldStop;
    NHashSet<CString> Breakpoints;
    CArray<CString> Errors;
    
    mutable NMutex ContextMutex;
};

/**
 * @brief NBP上下文实现
 */
class LIBNLIB_API NBPContext : public IScriptContext
{
public:
    NBPContext();
    virtual ~NBPContext();

    // IScriptContext接口实现
    virtual void SetGlobal(const CString& Name, const CScriptValue& Value) override;
    virtual CScriptValue GetGlobal(const CString& Name) const override;
    virtual bool HasGlobal(const CString& Name) const override;

    virtual void BindObject(const CString& Name, CObject* Object) override;
    virtual void UnbindObject(const CString& Name) override;

    virtual void BindFunction(const CString& Name, const NFunction<CScriptValue(const CArray<CScriptValue>&)>& Function) override;
    virtual void UnbindFunction(const CString& Name) override;

    virtual bool LoadModule(const CString& ModuleName, const CString& ModulePath) override;
    virtual bool UnloadModule(const CString& ModuleName) override;
    virtual CArray<CString> GetLoadedModules() const override;

    virtual NScriptResult Execute(const CString& Code) override;
    virtual NScriptResult ExecuteFile(const CString& FilePath) override;
    virtual NScriptResult CallFunction(const CString& FunctionName, const CArray<CScriptValue>& Args = {}) override;

    virtual void SetBreakpoint(const CString& FilePath, int32_t Line) override;
    virtual void RemoveBreakpoint(const CString& FilePath, int32_t Line) override;
    virtual void SetDebugMode(bool bEnabled) override;

    virtual void CollectGarbage() override;
    virtual size_t GetMemoryUsage() const override;

    // NBP特定接口
    bool LoadGraph(const CString& GraphPath);
    bool SaveGraph(const CString& GraphPath, NBPGraph* Graph);
    NBPGraph* GetLoadedGraph(const CString& GraphName) const;
    
    // 节点注册
    void RegisterNodeType(ENBPNodeType NodeType, const NFunction<TSharedPtr<NBPNode>()>& Factory);
    TSharedPtr<NBPNode> CreateNode(ENBPNodeType NodeType) const;
    
    // 执行环境
    CBPExecutionContext* GetExecutionContext() const { return ExecutionContext.Get(); }

private:
    CHashMap<CString, TSharedPtr<NBPGraph>> LoadedGraphs;
    CHashMap<ENBPNodeType, NFunction<TSharedPtr<NBPNode>()>> NodeFactories;
    TSharedPtr<CBPExecutionContext> ExecutionContext;
    
    // 全局状态
    CHashMap<CString, CScriptValue> GlobalVariables;
    CHashMap<CString, NFunction<CScriptValue(const CArray<CScriptValue>&)>> BoundFunctions;
    CHashMap<CString, CObject*> BoundObjects;
    
    // NBP解析
    TSharedPtr<NBPGraph> ParseNBPText(const CString& NBPCode);
    TSharedPtr<NBPNode> ParseNode(const CString& NodeText);
    NBPConnection ParseConnection(const CString& ConnectionText);
    
    mutable NMutex ContextMutex;
};

/**
 * @brief NBP引擎实现
 */
class LIBNLIB_API NBPEngine : public IScriptEngine
{
public:
    NBPEngine();
    virtual ~NBPEngine();

    // IScriptEngine接口实现
    virtual EScriptLanguage GetLanguage() const override { return EScriptLanguage::NBP; }
    virtual CString GetVersion() const override;
    virtual CString GetName() const override { return "NLib Blueprint Script Engine"; }

    virtual bool Initialize() override;
    virtual void Shutdown() override;
    virtual bool IsInitialized() const override { return bInitialized; }

    virtual TSharedPtr<IScriptContext> CreateContext() override;
    virtual void DestroyContext(TSharedPtr<IScriptContext> Context) override;
    virtual TSharedPtr<IScriptContext> GetMainContext() override;

    virtual bool RegisterClass(const CString& ClassName) override;
    virtual bool UnregisterClass(const CString& ClassName) override;
    virtual bool IsClassRegistered(const CString& ClassName) const override;

    virtual bool AutoBindClasses() override;
    virtual bool AutoBindClass(const CString& ClassName) override;

    virtual bool EnableHotReload(const CString& WatchDirectory) override;
    virtual void DisableHotReload() override;
    virtual bool IsHotReloadEnabled() const override { return bHotReloadEnabled; }

    virtual void ResetStatistics() override;
    virtual CHashMap<CString, double> GetStatistics() const override;

    // NBP特定功能
    void RegisterBuiltinNodes();
    void RegisterCustomNode(ENBPNodeType NodeType, const NFunction<TSharedPtr<NBPNode>()>& Factory);
    
    // 图表编译
    bool CompileGraph(const CString& GraphPath, const CString& OutputPath);
    bool OptimizeGraph(NBPGraph* Graph);
    
    // 可视化编辑器支持
    CString GenerateEditorMetadata(const CString& ClassName) const;
    CString GenerateNodePalette() const;

private:
    bool bInitialized;
    TSharedPtr<NBPContext> MainContext;
    CArray<TWeakPtr<NBPContext>> CreatedContexts;
    
    // 类注册信息
    NHashSet<CString> RegisteredClasses;
    
    // 热重载
    bool bHotReloadEnabled;
    CString WatchDirectory;
    TSharedPtr<CThread> HotReloadThread;
    NFileSystemWatcher FileWatcher;
    
    // 性能统计
    mutable NMutex StatsMutex;
    CHashMap<CString, double> Statistics;
    
    // 内置节点注册
    void RegisterControlFlowNodes();
    void RegisterMathNodes();
    void RegisterObjectNodes();
    void RegisterArrayNodes();
    void RegisterStringNodes();
    void RegisterEventNodes();
    void RegisterNetworkNodes();
    void RegisterDebugNodes();
    
    mutable NMutex EngineMutex;
};

/**
 * @brief NBP语法示例
 */
namespace NBPExamples
{
    // 基本图表示例
    extern const char* BasicGraphExample;
    
    // 函数定义示例
    extern const char* FunctionExample;
    
    // 事件处理示例
    extern const char* EventHandlerExample;
    
    // 网络消息处理示例
    extern const char* NetworkExample;
}

/**
 * @brief NBP文本语法示例
 * 
 * NBP支持文本描述的可视化脚本：
 * 
 * graph PlayerDamageHandler {
 *     variables {
 *         float maxHealth = 100.0;
 *         float currentHealth = 100.0;
 *     }
 *     
 *     nodes {
 *         entry Start {
 *             position: (0, 0);
 *             outputs: [exec];
 *         }
 *         
 *         event OnDamageReceived {
 *             position: (200, 0);
 *             inputs: [exec];
 *             outputs: [exec, float damageAmount];
 *         }
 *         
 *         math Subtract {
 *             position: (400, 100);
 *             inputs: [float a, float b];
 *             outputs: [float result];
 *         }
 *         
 *         branch CheckDeath {
 *             position: (600, 0);
 *             inputs: [exec, bool condition];
 *             outputs: [exec true, exec false];
 *         }
 *         
 *         function CallOnDeath {
 *             position: (800, -50);
 *             inputs: [exec];
 *             outputs: [exec];
 *             target: "OnPlayerDeath";
 *         }
 *         
 *         setProperty UpdateHealth {
 *             position: (800, 50);
 *             inputs: [exec, float value];
 *             outputs: [exec];
 *             property: "currentHealth";
 *         }
 *     }
 *     
 *     connections {
 *         Start.exec -> OnDamageReceived.exec;
 *         OnDamageReceived.exec -> Subtract.exec;
 *         OnDamageReceived.damageAmount -> Subtract.b;
 *         currentHealth -> Subtract.a;
 *         Subtract.result -> CheckDeath.condition;
 *         CheckDeath.true -> CallOnDeath.exec;
 *         CheckDeath.false -> UpdateHealth.exec;
 *         Subtract.result -> UpdateHealth.value;
 *     }
 * }
 */

} // namespace NLib