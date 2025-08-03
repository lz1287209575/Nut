#pragma once

/**
 * @file NLuaClassEngine.h
 * @brief NLux脚本引擎 - NLib Luxurious eXtended
 * 
 * NLux基于Lua 5.4深度魔改，专为游戏服务器设计的高级脚本引擎
 * 特性包括：原生类系统、接口、泛型、协程增强、JIT编译、类型检查
 * 让脚本编程更像现代面向对象语言，同时保持Lua的简洁和高性能
 */

#include "Scripting/NScriptEngine.h"
#include "Threading/CThread.h"
#include "FileSystem/NFileSystem.h"

// NLux引擎的魔改Lua headers
extern "C" {
#include "nlux.h"             // NLux核心引擎
#include "nlux_class.h"       // 类系统支持
#include "nlux_jit.h"         // JIT编译器
#include "nlux_coroutine.h"   // 增强协程
#include "nlux_types.h"       // 类型系统
}

namespace NLib
{

/**
 * @brief NLux语法扩展
 * 
 * NLux为Lua添加了完整的现代语言特性：
 * 
 * 1. 类定义:
 *    class Player : GameObject {
 *        public:
 *            string name;
 *            float health = 100.0;
 *            
 *        private:
 *            int level;
 *            
 *        public:
 *            constructor(string playerName) {
 *                this.name = playerName;
 *                this.health = 100.0;
 *            }
 *            
 *            virtual void takeDamage(float damage) {
 *                this.health -= damage;
 *            }
 *            
 *            override string toString() {
 *                return "Player: " + this.name;
 *            }
 *    }
 * 
 * 2. 接口定义:
 *    interface IDamageable {
 *        void takeDamage(float damage);
 *        float getHealth();
 *    }
 * 
 * 3. 泛型支持:
 *    class Container<TType> {
 *        private:
 *            TType[] items;
 *            
 *        public:
 *            void add(TType item) {
 *                this.items.push(item);
 *            }
 *    }
 * 
 * 4. 命名空间:
 *    namespace Game.Entities {
 *        class Player { ... }
 *    }
 * 
 * 5. 属性语法:
 *    class Player {
 *        private:
 *            string _name;
 *            
 *        public:
 *            property string name {
 *                get { return this._name; }
 *                set { this._name = value; }
 *            }
 *    }
 */

/**
 * @brief NLux的关键字扩展
 */
enum class ENLuxKeyword : uint32_t
{
    Class = 1 << 0,         // class
    Interface = 1 << 1,     // interface
    Namespace = 1 << 2,     // namespace
    Public = 1 << 3,        // public
    Private = 1 << 4,       // private
    Protected = 1 << 5,     // protected
    Virtual = 1 << 6,       // virtual
    Override = 1 << 7,      // override
    Abstract = 1 << 8,      // abstract
    Static = 1 << 9,        // static
    Constructor = 1 << 10,  // constructor
    Destructor = 1 << 11,   // destructor
    Property = 1 << 12,     // property
    Get = 1 << 13,          // get
    Set = 1 << 14,          // set
    This = 1 << 15,         // this
    Super = 1 << 16,        // super
    Typeof = 1 << 17,       // typeof
    Instanceof = 1 << 18,   // instanceof
    New = 1 << 19,          // new
    Delete = 1 << 20,       // delete
    Async = 1 << 21,        // async
    Await = 1 << 22,        // await
    Yield = 1 << 23,        // yield
    Using = 1 << 24,        // using
    Import = 1 << 25,       // import
    Export = 1 << 26        // export
};

/**
 * @brief NLux上下文实现
 */
class LIBNLIB_API NLuxContext : public IScriptContext
{
public:
    explicit NLuaClassContext(lua_State* InLuaState);
    virtual ~NLuaClassContext();

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

    // Lua Class特定接口
    lua_State* GetLuaState() const { return L; }
    
    // 类系统支持
    bool DefineClass(const CString& ClassName, const CString& BaseClass = "");
    bool DefineInterface(const CString& InterfaceName);
    bool DefineNamespace(const CString& NamespaceName);
    
    // 类成员定义
    bool AddClassMethod(const CString& ClassName, const CString& MethodName, 
                       const NFunction<CScriptValue(const CArray<CScriptValue>&)>& Method,
                       bool bIsVirtual = false, bool bIsStatic = false);
    bool AddClassProperty(const CString& ClassName, const CString& PropertyName,
                         const NFunction<CScriptValue()>& Getter,
                         const NFunction<void(const CScriptValue&)>& Setter = nullptr);
    
    // 泛型支持
    bool DefineGenericClass(const CString& ClassName, const CArray<CString>& TypeParameters);
    bool InstantiateGenericClass(const CString& ClassName, const CArray<CString>& TypeArguments);
    
    // 反射支持
    CArray<CString> GetClassMethods(const CString& ClassName) const;
    CArray<CString> GetClassProperties(const CString& ClassName) const;
    CArray<CString> GetClassInterfaces(const CString& ClassName) const;
    CString GetClassBaseClass(const CString& ClassName) const;

private:
    lua_State* L;
    
    // 类系统状态
    CHashMap<CString, CHashMap<CString, CString>> ClassHierarchy;    // ClassName -> BaseClass
    CHashMap<CString, CArray<CString>> ClassInterfaces;             // ClassName -> Interfaces
    CHashMap<CString, CArray<CString>> ClassMethods;                // ClassName -> Methods
    CHashMap<CString, CArray<CString>> ClassProperties;             // ClassName -> Properties
    CHashMap<CString, CArray<CString>> Namespaces;                  // Namespace -> Contents
    
    // 魔改Lua特定的C API包装
    int CreateClass(const CString& ClassName, const CString& BaseClass);
    int CreateInterface(const CString& InterfaceName);
    int AddMethodToClass(const CString& ClassName, const CString& MethodName, lua_CFunction Function);
    int AddPropertyToClass(const CString& ClassName, const CString& PropertyName, 
                          lua_CFunction Getter, lua_CFunction Setter);
    
    // 语法预处理器
    CString PreprocessClassSyntax(const CString& Code);
    CString TransformClassDefinition(const CString& ClassDef);
    CString TransformPropertyDefinition(const CString& PropertyDef);
    CString TransformMethodDefinition(const CString& MethodDef);
    
    mutable NMutex ContextMutex;
};

/**
 * @brief Lua Class引擎实现
 */
class LIBNLIB_API NLuaClassEngine : public IScriptEngine
{
public:
    NLuaClassEngine();
    virtual ~NLuaClassEngine();

    // IScriptEngine接口实现
    virtual EScriptLanguage GetLanguage() const override { return EScriptLanguage::LuaClass; }
    virtual CString GetVersion() const override;
    virtual CString GetName() const override { return "NLib Lua Class Engine"; }

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

    // Lua Class特定功能
    bool CompileToLuaBytecode(const CString& ClassCode, const CString& OutputPath);
    bool LoadCompiledBytecode(const CString& FilePath);
    
    // 类型系统
    bool EnableStrictTyping(bool bEnabled = true);
    bool EnableRuntimeTypeChecking(bool bEnabled = true);
    
    // 性能优化
    bool EnableJIT(bool bEnabled = true);      // Just-In-Time编译
    bool EnableInlining(bool bEnabled = true); // 方法内联
    
    // 互操作性
    bool ImportLuaModule(const CString& ModulePath);     // 导入标准Lua模块
    bool ExportToLua(const CString& ClassName);          // 导出类到标准Lua
    
private:
    bool bInitialized;
    TSharedPtr<NLuaClassContext> MainContext;
    CArray<TWeakPtr<NLuaClassContext>> CreatedContexts;
    
    // 类注册信息
    NHashSet<CString> RegisteredClasses;
    
    // 魔改特性开关
    bool bStrictTyping;
    bool bRuntimeTypeChecking;
    bool bJITEnabled;
    bool bInliningEnabled;
    
    // 热重载
    bool bHotReloadEnabled;
    CString WatchDirectory;
    TSharedPtr<CThread> HotReloadThread;
    NFileSystemWatcher FileWatcher;
    
    // 性能统计
    mutable NMutex StatsMutex;
    CHashMap<CString, double> Statistics;
    
    // 初始化辅助
    bool InitializeLuaClassState(lua_State* L);
    void SetupClassSystem(lua_State* L);
    void SetupOOPKeywords(lua_State* L);
    void SetupTypeSystem(lua_State* L);
    
    // 类绑定辅助
    bool BindLuaClassClass(const CString& ClassName, const NScriptClassMeta& ClassMeta);
    bool BindLuaClassProperty(lua_State* L, const CString& ClassName, 
                             const CString& PropertyName, const NScriptPropertyMeta& PropertyMeta);
    bool BindLuaClassFunction(lua_State* L, const CString& ClassName, 
                             const CString& FunctionName, const NScriptFunctionMeta& FunctionMeta);
    
    // 语法解析和转换
    CString ConvertCppClassToLuaClass(const CString& ClassName, const NScriptClassMeta& Meta);
    CString GenerateLuaClassDefinition(const CString& ClassName, const NScriptClassMeta& Meta);
    
    mutable NMutex EngineMutex;
};

/**
 * @brief Lua Class代码生成器
 */
class LIBNLIB_API NLuaClassCodeGenerator
{
public:
    // 从C++类生成Lua Class代码
    static CString GenerateClassDefinition(const CString& ClassName, const NScriptClassMeta& Meta);
    static CString GeneratePropertyDefinition(const CString& PropertyName, const NScriptPropertyMeta& Meta);
    static CString GenerateFunctionDefinition(const CString& FunctionName, const NScriptFunctionMeta& Meta);
    
    // 生成类型转换代码
    static CString GenerateTypeConversion(const CString& CppType);
    static CString GenerateDefaultValue(const CString& Type, const CString& DefaultValue);
    
    // 生成互操作代码
    static CString GenerateInteropWrapper(const CString& ClassName);
    static CString GenerateEventBinding(const CString& EventName);
    
private:
    static CString IndentCode(const CString& Code, int32_t IndentLevel = 1);
    static CString SanitizeIdentifier(const CString& Identifier);
};

/**
 * @brief Lua Class语法示例
 */
namespace LuaClassExamples
{
    // 基本类定义示例
    extern const char* BasicClassExample;
    
    // 继承示例
    extern const char* InheritanceExample;
    
    // 接口实现示例
    extern const char* InterfaceExample;
    
    // 泛型类示例
    extern const char* GenericClassExample;
    
    // 属性定义示例
    extern const char* PropertyExample;
    
    // 命名空间示例
    extern const char* NamespaceExample;
}

/**
 * @brief 示例生成的Lua Class代码
 * 
 * 从这个C++类：
 * NCLASS(ScriptCreatable, Category="Gameplay")
 * class CPlayer : public NGameObject {
 * public:
 *     NPROPERTY(ScriptReadWrite) float Health = 100.0f;
 *     NFUNCTION(ScriptCallable) void TakeDamage(float Amount);
 * };
 * 
 * 生成这样的Lua Class代码：
 * class Player : GameObject {
 *     public:
 *         property float health {
 *             get { return this._health; }
 *             set { this._health = value; }
 *         }
 *         
 *     private:
 *         float _health = 100.0;
 *         
 *     public:
 *         constructor() {
 *             super();
 *             this._health = 100.0;
 *         }
 *         
 *         virtual void takeDamage(float amount) {
 *             this._health -= amount;
 *             if (this._health <= 0) {
 *                 this._health = 0;
 *                 this:onDeath();
 *             }
 *         }
 *         
 *         protected virtual void onDeath() {
 *             -- 可以被子类重写
 *         }
 * }
 */

} // namespace NLib