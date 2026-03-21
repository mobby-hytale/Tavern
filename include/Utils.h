#pragma once

#include "SDK.h"

extern "C" {
	#include "lua/lua.h"
	#include "lua/lualib.h"
	#include "lua/lauxlib.h"
}

std::string GetName(UObject* Obj);
UObject* FindObject(std::string NameContains);
UObject* GetPlayerCharacter();
uintptr_t FindPropertyOffset(UObject* Obj, const std::string& propName);
std::string ReadUEName(uintptr_t objectBase);
bool IsPropertyType(const std::string& typeName);

void LuaRegister();
void LoadLuaMods();
void CheckLuaInputs();
void close_lua();
