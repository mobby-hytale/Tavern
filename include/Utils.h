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

void LuaRegister();
void LoadLuaMods();
void CheckLuaInputs();
void close_lua();
