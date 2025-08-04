/*
** $Id: lclass.h $
** LuaForge Class System
** See Copyright Notice in lua.h
*/

#ifndef lclass_h
#define lclass_h

#include "lobject.h"
#include "lstate.h"
#include "ltable.h"

/* Class system types are now defined in lobject.h */

/*
** Class system API functions
*/

/* Class creation and management */
LUAI_FUNC ClassDesc* luaC_newclass(lua_State* L, TString* name, ClassDesc* super);
LUAI_FUNC void luaC_addmethod(
    lua_State* L, ClassDesc* cd, TString* name, TValue* func, lu_byte visibility, lu_byte modifiers);
LUAI_FUNC void luaC_addfield(lua_State* L, ClassDesc* cd, TString* name, TValue* value, lu_byte visibility);
LUAI_FUNC void luaC_addproperty(
    lua_State* L, ClassDesc* cd, TString* name, TValue* getter, TValue* setter, lu_byte visibility);

/* Interface creation and management */
LUAI_FUNC InterfaceDesc* luaC_newinterface(lua_State* L, TString* name);
LUAI_FUNC void luaC_addinterfacemethod(lua_State* L, InterfaceDesc* id, TString* name, TValue* signature);

/* Namespace creation and management */
LUAI_FUNC NamespaceDesc* luaC_newnamespace(lua_State* L, TString* name, NamespaceDesc* parent);
LUAI_FUNC void luaC_addnamespacemember(lua_State* L, NamespaceDesc* nd, TString* name, TValue* value);

/* Instance creation and management */
LUAI_FUNC ClassInstance* luaC_newinstance(lua_State* L, ClassDesc* cd);
LUAI_FUNC int luaC_getfield(lua_State* L, ClassInstance* inst, TString* name);
LUAI_FUNC void luaC_setfield(lua_State* L, ClassInstance* inst, TString* name, TValue* value);

/* Class system utilities */
LUAI_FUNC int luaC_isinstance(ClassInstance* inst, ClassDesc* cd);
LUAI_FUNC int luaC_implements(ClassDesc* cd, InterfaceDesc* id);
LUAI_FUNC ClassMember* luaC_findmember(ClassDesc* cd, TString* name, lu_byte visibility);
LUAI_FUNC int luaC_checkvisibility(ClassDesc* cd, ClassMember* member, ClassDesc* caller);

/* Type checking */
LUAI_FUNC int luaC_isclass(TValue* o);
LUAI_FUNC int luaC_isinterface(TValue* o);
LUAI_FUNC int luaC_isnamespace(TValue* o);
LUAI_FUNC int luaC_isclassinstance(TValue* o);

/* Garbage collection support */
LUAI_FUNC void luaC_markclass(global_State* g, ClassDesc* cd);
LUAI_FUNC void luaC_markinterface(global_State* g, InterfaceDesc* id);
LUAI_FUNC void luaC_marknamespace(global_State* g, NamespaceDesc* nd);
LUAI_FUNC void luaC_markinstance(global_State* g, ClassInstance* inst);

LUAI_FUNC size_t luaC_sizeclass(ClassDesc* cd);
LUAI_FUNC size_t luaC_sizeinterface(InterfaceDesc* id);
LUAI_FUNC size_t luaC_sizenamespace(NamespaceDesc* nd);
LUAI_FUNC size_t luaC_sizeinstance(ClassInstance* inst);

/* Metamethods for class system */
LUAI_FUNC const TValue* luaC_getclassmeta(lua_State* L, ClassDesc* cd, TMS event);
LUAI_FUNC void luaC_callconstructor(lua_State* L, ClassInstance* inst, int nargs);
LUAI_FUNC void luaC_calldestructor(lua_State* L, ClassInstance* inst);

/* Class registration with global state */
LUAI_FUNC void luaC_registerclass(lua_State* L, ClassDesc* cd);
LUAI_FUNC void luaC_registerinterface(lua_State* L, InterfaceDesc* id);
LUAI_FUNC void luaC_registernamespace(lua_State* L, NamespaceDesc* nd);

LUAI_FUNC ClassDesc* luaC_findclass(lua_State* L, TString* name);
LUAI_FUNC InterfaceDesc* luaC_findinterface(lua_State* L, TString* name);
LUAI_FUNC NamespaceDesc* luaC_findnamespace(lua_State* L, TString* name);

/* Error handling */
LUAI_FUNC l_noret luaC_classerror(lua_State* L, const char* msg, TString* name);

#endif