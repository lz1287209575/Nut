#include <stdio.h>
#include "src/lua.h"
#include "src/lualib.h"
#include "src/lauxlib.h"
#include "src/lclass.h"

int main() {
    printf("LuaForge Compilation Test\n");
    printf("Version: %s\n", LUA_RELEASE);
    
    // Test if our new types are defined
    printf("New types defined:\n");
    printf("- LUA_TCLASS: %d\n", LUA_TCLASS);
    printf("- LUA_TINTERFACE: %d\n", LUA_TINTERFACE);
    printf("- LUA_TNAMESPACE: %d\n", LUA_TNAMESPACE);
    printf("- LUA_NUMTYPES: %d\n", LUA_NUMTYPES);
    
    // Test if new keywords are defined
    printf("New keywords defined:\n");
    printf("- TK_CLASS: %d\n", TK_CLASS);
    printf("- TK_INTERFACE: %d\n", TK_INTERFACE);
    printf("- TK_NAMESPACE: %d\n", TK_NAMESPACE);
    printf("- TK_PUBLIC: %d\n", TK_PUBLIC);
    printf("- TK_PRIVATE: %d\n", TK_PRIVATE);
    printf("- TK_CONSTRUCTOR: %d\n", TK_CONSTRUCTOR);
    
    lua_State *L = luaL_newstate();
    if (L == NULL) {
        printf("Failed to create Lua state\n");
        return 1;
    }
    
    luaL_openlibs(L);
    
    printf("LuaForge state created successfully!\n");
    printf("LuaForge class system ready.\n");
    
    lua_close(L);
    return 0;
}