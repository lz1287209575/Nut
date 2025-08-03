#pragma once

/**
 * @file NScriptBindingGenerator.h
 * @brief 自动脚本绑定生成器
 * 
 * 基于NCLASS反射元数据自动生成脚本绑定代码
 * 这个文件主要用于NutHeaderTools工具，在编译时生成绑定代码
 */

#include "Core/CObject.h"
#include "Scripting/NScriptMeta.h"
#include "Containers/CString.h"
#include "Containers/CArray.h"
#include "Containers/CHashMap.h"

namespace NLib
{

/**
 * @brief 脚本绑定代码生成器
 */
class LIBNLIB_API NScriptBindingGenerator
{
public:
    struct PropertyInfo
    {
        CString Name;
        CString Type;
        CString Getter;
        CString Setter;
        NScriptPropertyMeta Meta;
        bool bHasGetter;
        bool bHasSetter;
    };

    struct FunctionInfo
    {
        CString Name;
        CString ReturnType;
        CString Signature;
        CArray<CString> ParameterTypes;
        CArray<CString> ParameterNames;
        NScriptFunctionMeta Meta;
        bool bIsStatic;
        bool bIsConst;
    };

    struct ClassInfo
    {
        CString Name;
        CString BaseClass;
        NScriptClassMeta Meta;
        CArray<PropertyInfo> Properties;
        CArray<FunctionInfo> Functions;
        bool bHasDefaultConstructor;
        bool bIsAbstract;
    };

public:
    NScriptBindingGenerator();
    virtual ~NScriptBindingGenerator() = default;

    // 解析源代码中的脚本元数据
    bool ParseSourceFile(const CString& FilePath);
    bool ParseSourceCode(const CString& Code, const CString& FilePath = "");

    // 添加类信息
    void AddClass(const ClassInfo& Info);
    void AddProperty(const CString& ClassName, const PropertyInfo& Info);
    void AddFunction(const CString& ClassName, const FunctionInfo& Info);

    // 生成绑定代码
    CString GenerateLuaBindings() const;
    CString GeneratePythonBindings() const;
    CString GenerateJavaScriptBindings() const;

    // 生成元数据注册代码
    CString GenerateMetaRegistration() const;

    // 生成完整的.generate.h文件
    CString GenerateHeaderFile(const CString& OriginalHeaderPath) const;

    // 工具函数
    void Clear();
    bool HasClass(const CString& ClassName) const;
    const ClassInfo* GetClass(const CString& ClassName) const;
    CArray<CString> GetScriptAccessibleClasses(EScriptLanguage Language) const;

private:
    CHashMap<CString, ClassInfo> Classes;

    // 代码生成辅助函数
    CString GenerateLuaClassBinding(const ClassInfo& Class) const;
    CString GenerateLuaPropertyBinding(const CString& ClassName, const PropertyInfo& Property) const;
    CString GenerateLuaFunctionBinding(const CString& ClassName, const FunctionInfo& Function) const;

    CString GeneratePythonClassBinding(const ClassInfo& Class) const;
    CString GeneratePythonPropertyBinding(const CString& ClassName, const PropertyInfo& Property) const;
    CString GeneratePythonFunctionBinding(const CString& ClassName, const FunctionInfo& Function) const;

    CString GenerateJSClassBinding(const ClassInfo& Class) const;
    CString GenerateJSPropertyBinding(const CString& ClassName, const PropertyInfo& Property) const;
    CString GenerateJSFunctionBinding(const CString& ClassName, const FunctionInfo& Function) const;

    // 元数据解析
    NScriptClassMeta ParseClassMeta(const CString& MetaString) const;
    NScriptPropertyMeta ParsePropertyMeta(const CString& MetaString) const;
    NScriptFunctionMeta ParseFunctionMeta(const CString& MetaString) const;

    // 代码解析辅助
    CString ExtractClassName(const CString& ClassDeclaration) const;
    CString ExtractBaseClass(const CString& ClassDeclaration) const;
    CArray<PropertyInfo> ExtractProperties(const CString& ClassBody) const;
    CArray<FunctionInfo> ExtractFunctions(const CString& ClassBody) const;

    // C++类型到脚本类型转换
    CString CppTypeToLuaType(const CString& CppType) const;
    CString CppTypeToPythonType(const CString& CppType) const;  
    CString CppTypeToJSType(const CString& CppType) const;

    // 生成器选项
    bool bGenerateDebugInfo;
    bool bGenerateDocumentation;
    bool bOptimizeForSize;
};

/**
 * @brief 脚本绑定模板
 */
class LIBNLIB_API NScriptBindingTemplates
{
public:
    // Lua绑定模板
    static const char* LuaClassTemplate;
    static const char* LuaPropertyGetterTemplate;
    static const char* LuaPropertySetterTemplate;
    static const char* LuaFunctionTemplate;
    static const char* LuaConstructorTemplate;

    // Python绑定模板
    static const char* PythonClassTemplate;
    static const char* PythonPropertyTemplate;
    static const char* PythonFunctionTemplate;
    static const char* PythonModuleTemplate;

    // JavaScript绑定模板
    static const char* JSClassTemplate;
    static const char* JSPropertyTemplate;
    static const char* JSFunctionTemplate;
    static const char* JSModuleTemplate;

    // 元数据注册模板
    static const char* MetaRegistrationTemplate;
    static const char* ClassMetaTemplate;
    static const char* PropertyMetaTemplate;
    static const char* FunctionMetaTemplate;
};

/**
 * @brief 示例生成的绑定代码
 * 
 * 以下是一个示例类如何生成脚本绑定：
 * 
 * 原始C++代码：
 * NCLASS(ScriptCreatable, Category="Gameplay", Description="Game player")
 * class CPlayer : public CObject
 * {
 * public:
 *     NPROPERTY(ScriptReadWrite, Description="Player health")
 *     float Health = 100.0f;
 *     
 *     NFUNCTION(ScriptCallable, Description="Take damage")
 *     void TakeDamage(float Amount);
 * };
 * 
 * 生成的Lua绑定：
 * static int NPlayer_GetHealth(lua_State* L) {
 *     CPlayer* self = GetObjectFromLua<CPlayer>(L, 1);
 *     lua_pushnumber(L, self->Health);
 *     return 1;
 * }
 * 
 * static int NPlayer_SetHealth(lua_State* L) {
 *     CPlayer* self = GetObjectFromLua<CPlayer>(L, 1);
 *     self->Health = luaL_checknumber(L, 2);
 *     return 0;
 * }
 * 
 * static int NPlayer_TakeDamage(lua_State* L) {
 *     CPlayer* self = GetObjectFromLua<CPlayer>(L, 1);
 *     float Amount = luaL_checknumber(L, 2);
 *     self->TakeDamage(Amount);
 *     return 0;
 * }
 * 
 * void RegisterNPlayerToLua(lua_State* L) {
 *     luaL_newmetatable(L, "CPlayer");
 *     
 *     lua_pushstring(L, "Health");
 *     lua_pushcfunction(L, NPlayer_GetHealth);
 *     lua_settable(L, -3);
 *     
 *     lua_pushstring(L, "SetHealth");
 *     lua_pushcfunction(L, NPlayer_SetHealth);
 *     lua_settable(L, -3);
 *     
 *     lua_pushstring(L, "TakeDamage");
 *     lua_pushcfunction(L, NPlayer_TakeDamage);
 *     lua_settable(L, -3);
 *     
 *     lua_pop(L, 1);
 * }
 */

} // namespace NLib