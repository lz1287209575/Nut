/*
** $Id: lclass.c $
** LuaForge Class System Implementation
** See Copyright Notice in lua.h
*/

#define lclass_c
#define LUA_CORE

#include "lprefix.h"

#include <string.h>

#include "lua.h"

#include "lapi.h"
#include "lclass.h"
#include "ldebug.h"
#include "ldo.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"

/*
** Class creation and management
*/

ClassDesc *luaC_newclass(lua_State *L, TString *name, ClassDesc *super) {
  global_State *g = G(L);
  ClassDesc *cd = cast(ClassDesc *, luaM_newobject(L, LUA_TCLASS, sizeof(ClassDesc)));
  cd->name = name;
  cd->super = super;
  cd->interfaces = luaH_new(L);
  cd->members = NULL;
  cd->metamethods = luaH_new(L);
  cd->static_members = luaH_new(L);
  cd->modifiers = CLASS_MODIFIER_NONE;
  cd->member_count = 0;
  
  /* Register in global class registry */
  luaC_registerclass(L, cd);
  
  return cd;
}

void luaC_addmethod(lua_State *L, ClassDesc *cd, TString *name, 
                    TValue *func, lu_byte visibility, lu_byte modifiers) {
  ClassMember *member = luaM_new(L, ClassMember);
  member->name = name;
  member->type = CLASS_MEMBER_METHOD;
  member->visibility = visibility;
  member->modifiers = modifiers;
  setobj(L, &member->value, func);
  
  /* Add to member chain */
  member->next = cd->members;
  cd->members = member;
  cd->member_count++;
}

void luaC_addfield(lua_State *L, ClassDesc *cd, TString *name, 
                   TValue *value, lu_byte visibility) {
  ClassMember *member = luaM_new(L, ClassMember);
  member->name = name;
  member->type = CLASS_MEMBER_FIELD;
  member->visibility = visibility;
  member->modifiers = CLASS_MODIFIER_NONE;
  setobj(L, &member->value, value);
  
  /* Add to member chain */
  member->next = cd->members;
  cd->members = member;
  cd->member_count++;
}

void luaC_addproperty(lua_State *L, ClassDesc *cd, TString *name,
                      TValue *getter, TValue *setter, lu_byte visibility) {
  ClassMember *member = luaM_new(L, ClassMember);
  member->name = name;
  member->type = CLASS_MEMBER_PROPERTY;
  member->visibility = visibility;
  member->modifiers = CLASS_MODIFIER_NONE;
  
  /* Create property table with getter/setter */
  Table *prop = luaH_new(L);
  if (getter && !ttisnil(getter)) {
    TValue k;
    setsvalue(L, &k, luaS_newliteral(L, "get"));
    luaH_set(L, prop, &k, getter);
  }
  if (setter && !ttisnil(setter)) {
    TValue k;
    setsvalue(L, &k, luaS_newliteral(L, "set"));
    luaH_set(L, prop, &k, setter);
  }
  sethvalue(L, &member->value, prop);
  
  /* Add to member chain */
  member->next = cd->members;
  cd->members = member;
  cd->member_count++;
}

/*
** Interface creation and management
*/

InterfaceDesc *luaC_newinterface(lua_State *L, TString *name) {
  InterfaceDesc *id = cast(InterfaceDesc *, luaM_newobject(L, LUA_TINTERFACE, sizeof(InterfaceDesc)));
  id->name = name;
  id->methods = luaH_new(L);
  id->extends = luaH_new(L);
  
  /* Register in global interface registry */
  luaC_registerinterface(L, id);
  
  return id;
}

void luaC_addinterfacemethod(lua_State *L, InterfaceDesc *id, 
                            TString *name, TValue *signature) {
  TValue k;
  setsvalue(L, &k, name);
  luaH_set(L, id->methods, &k, signature);
}

/*
** Namespace creation and management
*/

NamespaceDesc *luaC_newnamespace(lua_State *L, TString *name, NamespaceDesc *parent) {
  NamespaceDesc *nd = cast(NamespaceDesc *, luaM_newobject(L, LUA_TNAMESPACE, sizeof(NamespaceDesc)));
  nd->name = name;
  nd->members = luaH_new(L);
  nd->parent = parent;
  
  /* Register in global namespace registry */
  luaC_registernamespace(L, nd);
  
  return nd;
}

void luaC_addnamespacemember(lua_State *L, NamespaceDesc *nd, 
                            TString *name, TValue *value) {
  TValue k;
  setsvalue(L, &k, name);
  luaH_set(L, nd->members, &k, value);
}

/*
** Instance creation and management
*/

ClassInstance *luaC_newinstance(lua_State *L, ClassDesc *cd) {
  ClassInstance *inst = cast(ClassInstance *, luaM_newobject(L, LUA_TUSERDATA, sizeof(ClassInstance)));
  inst->class_desc = cd;
  inst->fields = luaH_new(L);
  inst->ref_count = 1;
  
  /* Initialize fields with default values */
  ClassMember *member = cd->members;
  while (member) {
    if (member->type == CLASS_MEMBER_FIELD) {
      TValue k;
      setsvalue(L, &k, member->name);
      luaH_set(L, inst->fields, &k, &member->value);
    }
    member = member->next;
  }
  
  return inst;
}

int luaC_getfield(lua_State *L, ClassInstance *inst, TString *name) {
  TValue k;
  setsvalue(L, &k, name);
  const TValue *v = luaH_get(inst->fields, &k);
  if (!ttisnil(v)) {
    setobj2s(L, L->top.p, v);
    api_incr_top(L);
    return 1;
  }
  
  /* Check class members */
  ClassMember *member = luaC_findmember(inst->class_desc, name, CLASS_VISIBILITY_PUBLIC);
  if (member) {
    if (member->type == CLASS_MEMBER_METHOD) {
      setobj2s(L, L->top.p, &member->value);
      api_incr_top(L);
      return 1;
    } else if (member->type == CLASS_MEMBER_PROPERTY) {
      /* Call getter */
      struct Table *prop = hvalue(&member->value);
      TValue gk;
      setsvalue(L, &gk, luaS_newliteral(L, "get"));
      const TValue *getter = luaH_get(prop, &gk);
      if (!ttisnil(getter)) {
        setobj2s(L, L->top.p, getter);
        api_incr_top(L);
        setobj2s(L, L->top.p, cast(TValue *, inst));
        api_incr_top(L);
        luaD_call(L, L->top.p - 2, 1);
        return 1;
      }
    }
  }
  
  return 0;
}

void luaC_setfield(lua_State *L, ClassInstance *inst, TString *name, TValue *value) {
  /* Check if it's a field or property */
  ClassMember *member = luaC_findmember(inst->class_desc, name, CLASS_VISIBILITY_PUBLIC);
  if (member) {
    if (member->type == CLASS_MEMBER_FIELD) {
      TValue k;
      setsvalue(L, &k, name);
      luaH_set(L, inst->fields, &k, value);
    } else if (member->type == CLASS_MEMBER_PROPERTY) {
      /* Call setter */
      struct Table *prop = hvalue(&member->value);
      TValue sk;
      setsvalue(L, &sk, luaS_newliteral(L, "set"));
      const TValue *setter = luaH_get(prop, &sk);
      if (!ttisnil(setter)) {
        setobj2s(L, L->top.p, setter);
        api_incr_top(L);
        setobj2s(L, L->top.p, cast(TValue *, inst));
        api_incr_top(L);
        setobj2s(L, L->top.p, value);
        api_incr_top(L);
        luaD_call(L, L->top.p - 3, 0);
      }
    }
  } else {
    /* Dynamic field assignment */
    TValue k;
    setsvalue(L, &k, name);
    luaH_set(L, inst->fields, &k, value);
  }
}

/*
** Utility functions
*/

ClassMember *luaC_findmember(ClassDesc *cd, TString *name, lu_byte visibility) {
  ClassMember *member = cd->members;
  while (member) {
    if (eqshrstr(member->name, name)) {
      /* Check visibility */
      if (member->visibility == CLASS_VISIBILITY_PUBLIC || 
          member->visibility == visibility) {
        return member;
      }
    }
    member = member->next;
  }
  
  /* Search in superclass */
  if (cd->super) {
    return luaC_findmember(cd->super, name, visibility);
  }
  
  return NULL;
}

int luaC_isinstance(ClassInstance *inst, ClassDesc *cd) {
  ClassDesc *current = inst->class_desc;
  while (current) {
    if (current == cd) return 1;
    current = current->super;
  }
  return 0;
}

/*
** Type checking functions
*/

int luaC_isclass(TValue *o) {
  return ttisclass(o);
}

int luaC_isinterface(TValue *o) {
  return ttisinterface(o);
}

int luaC_isnamespace(TValue *o) {
  return ttisnamespace(o);
}

int luaC_isinstancevalue(TValue *o) {
  return ttisfulluserdata(o) && uvalue(o)->metatable == NULL; /* Class instances have no metatable by default */
}

/*
** Registration functions
*/

void luaC_registerclass(lua_State *L, ClassDesc *cd) {
  global_State *g = G(L);
  /* Add to global class registry - implementation depends on global state structure */
  /* For now, this is a placeholder */
}

ClassDesc *luaC_findclass(lua_State *L, TString *name) {
  /* For now, return NULL - implementation depends on global class registry */
  /* This is a placeholder implementation */
  return NULL;
}

void luaC_registerinterface(lua_State *L, InterfaceDesc *id) {
  global_State *g = G(L);
  /* Add to global interface registry */
}

void luaC_registernamespace(lua_State *L, NamespaceDesc *nd) {
  global_State *g = G(L);
  /* Add to global namespace registry */
}

/*
** Error handling
*/

l_noret luaC_classerror(lua_State *L, const char *msg, TString *name) {
  const char *namestr = name ? getstr(name) : "?";
  luaG_runerror(L, "%s '%s'", msg, namestr);
}