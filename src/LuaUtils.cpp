#include <windows.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <map>
#include <string>
#include "Scanner.h"
#include "minhook/MinHook.h"
#include "SDK.h"
#include "Utils.h"

extern "C" {
	#include "lua/lua.h"
	#include "lua/lualib.h"
	#include "lua/lauxlib.h"
}

lua_State* L = nullptr;

namespace fs = std::filesystem;

std::map<int, int> keyBinds;

static void PushUObject(lua_State* L, UObject* obj) {
	UObject** ud = (UObject**)lua_newuserdata(L, sizeof(UObject*));
	*ud = obj;
	luaL_getmetatable(L, "UObjectMeta");
	lua_setmetatable(L, -2);
}

static bool MatchesTarget(UObject* obj, const std::string& target) {
	if (!obj || (uintptr_t)obj < 0x10000) return false;
	std::string name = GetName(obj);
	if (name.find("Default__") != std::string::npos) return false;
	std::string nameLower = name;
	std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
	return nameLower.find(target) != std::string::npos;
}

int Lua_FindFirstOf(lua_State* L) {
	const char* targetName = luaL_checkstring(L, 1);
	if (!targetName || !GObjects) return 0;

	std::string target(targetName);
	std::transform(target.begin(), target.end(), target.begin(), ::tolower);

	for (int i = 0; i < GObjects->NumElements; i++) {
		UObject* obj = GObjects->GetObjectById(i);
		if (!MatchesTarget(obj, target)) continue;
		PushUObject(L, obj);
		return 1;
	}
	return 0;
}

int Lua_FindLastOf(lua_State* L) {
	const char* targetName = luaL_checkstring(L, 1);
	if (!targetName || !GObjects) return 0;

	std::string target(targetName);
	std::transform(target.begin(), target.end(), target.begin(), ::tolower);

	UObject* last = nullptr;
	for (int i = 0; i < GObjects->NumElements; i++) {
		UObject* obj = GObjects->GetObjectById(i);
		if (MatchesTarget(obj, target)) last = obj;
	}

	if (!last) return 0;
	PushUObject(L, last);
	return 1;
}

int Lua_FindAllOf(lua_State* L) {
	const char* targetName = luaL_checkstring(L, 1);
	if (!targetName || !GObjects) return 0;

	std::string target(targetName);
	std::transform(target.begin(), target.end(), target.begin(), ::tolower);

	lua_newtable(L);
	int count = 0;

	for (int i = 0; i < GObjects->NumElements; i++) {
		UObject* obj = GObjects->GetObjectById(i);
		if (!MatchesTarget(obj, target)) continue;
		count++;
		PushUObject(L, obj);
		lua_rawseti(L, -2, count);
	}

	return 1;
}

int Lua_Inspect(lua_State* L) {
	UObject** ud = (UObject**)luaL_checkudata(L, 1, "UObjectMeta");
	if (!ud || !*ud) return 0;
	UObject* Obj = *ud;

	UClass* Class = Obj->ClassPrivate;
	if (!IS_VALID(Class)) return 0;

	uintptr_t CurrentStruct = (uintptr_t)Class;
	int depth = 0;

	while (IS_VALID(CurrentStruct) && depth < 16) {
		depth++;

		std::string structName = ReadUEName(CurrentStruct);
		printf("\n  ---- [%s] ----\n", structName.c_str());

		uintptr_t Prop = *(uintptr_t*)(CurrentStruct + UE422::UStruct_Children);
		int propCount = 0;

		while (IS_VALID(Prop) && propCount < 256) {
			propCount++;

			std::string propName = ReadUEName(Prop);
			if (propName.empty() || propName == "None") {
				Prop = *(uintptr_t*)(Prop + UE422::UField_Next);
				continue;
			}

			uintptr_t fieldClassPtr = *(uintptr_t*)(Prop + UE422::UObject_ClassPrivate);
			std::string typeName = IS_VALID(fieldClassPtr)
				? ReadUEName(fieldClassPtr)
				: "Unknown";

			if (IsPropertyType(typeName)) {
				int32_t offset = *(int32_t*)(Prop + UE422::UProperty_Offset_Internal);

				char valueStr[64] = "?";
				uintptr_t valueAddr = (uintptr_t)Obj + offset;

				if (!IsBadReadPtr((void*)valueAddr, 8)) {
					if (typeName == "FloatProperty") {
						snprintf(valueStr, sizeof(valueStr), "%.4f", *(float*)valueAddr);
					} else if (typeName == "IntProperty") {
						snprintf(valueStr, sizeof(valueStr), "%d", *(int32_t*)valueAddr);
					} else if (typeName == "BoolProperty") {
						snprintf(valueStr, sizeof(valueStr), "%s", *(bool*)valueAddr ? "true" : "false");
					} else if (typeName == "ObjectProperty" || typeName == "ClassProperty") {
						uintptr_t ptr = *(uintptr_t*)valueAddr;
						snprintf(valueStr, sizeof(valueStr), "0x%llX", (unsigned long long)ptr);
					} else {
						uint64_t raw = *(uint64_t*)valueAddr;
						snprintf(valueStr, sizeof(valueStr), "raw=0x%llX", (unsigned long long)raw);
					}
				}

				printf("    [0x%03X] %-30s | %-20s | %s\n",
					offset, propName.c_str(), typeName.c_str(), valueStr);
			} else {
				printf("    [ fn ] %-30s | %s\n", propName.c_str(), typeName.c_str());
			}

			Prop = *(uintptr_t*)(Prop + UE422::UField_Next);
		}

		uintptr_t Super = *(uintptr_t*)(CurrentStruct + UE422::UStruct_SuperStruct);
		if (!IS_VALID(Super) || Super == CurrentStruct) break;
		std::string superName = ReadUEName(Super);
		if (superName == "Object") break;
		CurrentStruct = Super;
	}
	return 0;
}

int Lua_RegisterEventHook(lua_State* L) {
	const char* funcName = luaL_checkstring(L, 1);
	if (!lua_isfunction(L, 2)) return 0;
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	eventHooks[funcName] = ref;
	return 0;
}

int Lua_ObjectNewIndex(lua_State* L) {
	UObject** objPtr = (UObject**)luaL_checkudata(L, 1, "UObjectMeta");
	
	if (!objPtr || !*objPtr) return 0;

	UObject* obj = *objPtr;
	const char* propName = luaL_checkstring(L, 2);
	float value = (float)luaL_checknumber(L, 3);

	uintptr_t offset = FindPropertyOffset(obj, propName);

	if (offset > 0) {
		uintptr_t targetAddr = (uintptr_t)obj + offset;

		if (!IsBadWritePtr((LPVOID)targetAddr, sizeof(float))) {
			*(float*)targetAddr = value;
		}
	} else {
		std::cout << "[Tavern] Error: Property " << propName << " not found on " << GetName(obj) << std::endl;
	}
	return 0;
}

int Lua_ObjectIndex(lua_State* L) {
	UObject** objPtr = (UObject**)luaL_checkudata(L, 1, "UObjectMeta");
	const char* propName = luaL_checkstring(L, 2);
	if (!objPtr || !*objPtr) return 0;

	uintptr_t offset = FindPropertyOffset(*objPtr, propName);
	if (offset == 0) {
		lua_pushnil(L);
		return 1;
	}

	int val = *(int*)((uintptr_t)*objPtr + offset);
	lua_pushinteger(L, val);
	return 1;
}

UObject* GetAddressFromLua(lua_State* L, int idx) {
	if (lua_isuserdata(L, idx)) {
		UObject** ud = (UObject**)lua_touserdata(L, idx);
		return *ud;
	}
	return (UObject*)lua_touserdata(L, idx);
}

int Lua_GetName(lua_State* L) {
	UObject** ud = (UObject**)luaL_checkudata(L, 1, "UObjectMeta");
	if (!ud || !*ud) { lua_pushstring(L, "InvalidObject"); return 1; }
	std::string name = ReadUEName((uintptr_t)*ud);
	lua_pushstring(L, name.empty() ? "None" : name.c_str());
	return 1;
}

int Lua_ReadFloat(lua_State* L) {
	UObject** ud = (UObject**)luaL_checkudata(L, 1, "UObjectMeta");
	int offset = (int)luaL_checkinteger(L, 2);

	if (ud && *ud) {
		uintptr_t addr = (uintptr_t)*ud + offset;
		
		if (!IsBadReadPtr((void*)addr, sizeof(float))) {
			float val = *(float*)addr;
			lua_pushnumber(L, (double)val);
			return 1;
		}
	}

	return 0;
}

int Lua_ReadInt(lua_State* L) {
	UObject** ud = (UObject**)luaL_checkudata(L, 1, "UObjectMeta");
	int offset = (int)luaL_checkinteger(L, 2);

	if (ud && *ud) {
		uintptr_t addr = (uintptr_t)*ud + offset;
		
		if (!IsBadReadPtr((void*)addr, sizeof(int))) {
			int val = *(int*)addr;
			lua_pushinteger(L, val);
			return 1;
		}
	}

	return 0;
}

int Lua_WriteFloat(lua_State* L) {
	void* obj = lua_touserdata(L, 1);
	uintptr_t offset = (uintptr_t)luaL_checkinteger(L, 2);
	float val = (float)luaL_checknumber(L, 3);

	if (obj && (uintptr_t)obj > 0x10000) {
		uintptr_t addr = (uintptr_t)obj + offset;

		if (!IsBadWritePtr((LPVOID)addr, sizeof(float))) {
			*(float*)addr = val;
			return 0;
		}
		else {
			std::cout << "[Tavern] Erreur : Adresse memoire protegee a 0x" << std::hex << addr << std::endl;
		}
	}
	return 0;
}

int Lua_GetArrayElement(lua_State* L) {
	void* arrayPtr = lua_touserdata(L, 1);
	int index = (int)luaL_checkinteger(L, 2);
	int elementSize = 8;

	if (arrayPtr) {
		void* dataAddr = *(void**)arrayPtr;
		if (dataAddr) {
			void* element = *(void**)((uintptr_t)dataAddr + (index * elementSize));
			lua_pushlightuserdata(L, element);
			return 1;
		}
	}
	return 0;
}

int Lua_RegisterKeyBind(lua_State* L) {
	int key = (int)luaL_checkinteger(L, 1);
	if (lua_isfunction(L, 2)) {
		int ref = luaL_ref(L, LUA_REGISTRYINDEX);
		keyBinds[key] = ref;
		return 0;
	}
	return 0;
}

int Lua_ReadPtr(lua_State* L) {
	uintptr_t base = (uintptr_t)lua_touserdata(L, 1);
	uintptr_t offset = (uintptr_t)luaL_checkinteger(L, 2);
	if (base) {
		lua_pushlightuserdata(L, *(void**)(base + offset));
		return 1;
	}
	return 0;
}

int Lua_GetGEngine(lua_State* L) {
	uintptr_t addrGE = Scanner::FindPattern("48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 74 05");
	if (addrGE) {
		int32_t offset = *(int32_t*)(addrGE + 3);
		void* pGEngine = *(void**)(addrGE + 7 + offset);
		lua_pushlightuserdata(L, pGEngine);
		return 1;
	}
	return 0;
}

void LoadLuaMods() {
	const std::string modsDir = "Mods";

	if (!fs::exists(modsDir) || !fs::is_directory(modsDir)) {
		std::cout << "[Tavern] Mods folder not found" << std::endl;
		return;
	}

	for (const auto& entry : fs::directory_iterator(modsDir)) {
		if (!entry.is_directory()) continue;

		std::string modName = entry.path().filename().string();
		std::string scriptPath = entry.path().string() + "\\main.lua";

		if (!fs::exists(scriptPath)) {
			std::cout << "[Tavern] " << modName << ": no main.lua, skipped" << std::endl;
			continue;
		}

		if (luaL_dofile(L, scriptPath.c_str()) != LUA_OK) {
			std::cout << "[Lua Error] " << modName << ": " << lua_tostring(L, -1) << std::endl;
			lua_pop(L, 1);
		} else {
			std::cout << "[Tavern] " << modName << " loaded" << std::endl;
		}
	}
}

void CheckLuaInputs() {
	if (!GObjects || !L) return;

	for (auto const& [key, ref] : keyBinds) {
		if (GetAsyncKeyState(key) & 1) {
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
			if (lua_isfunction(L, -1)) {
				if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
					std::cout << "[Lua Error] " << lua_tostring(L, -1) << std::endl;
					lua_pop(L, 1);
				}
			}
			else {
				lua_pop(L, 1);
			}
		}
	}
}

void LuaKeyRegister(lua_State* L) {
	auto setKey = [&](const char* name, int vk) {
		lua_pushinteger(L, vk);
		lua_setglobal(L, name);
	};

	setKey("MOUSE_LEFT",   VK_LBUTTON);
	setKey("MOUSE_RIGHT",  VK_RBUTTON);
	setKey("MOUSE_MIDDLE", VK_MBUTTON);

	setKey("KEY_F1",  VK_F1);  setKey("KEY_F2",  VK_F2);
	setKey("KEY_F3",  VK_F3);  setKey("KEY_F4",  VK_F4);
	setKey("KEY_F5",  VK_F5);  setKey("KEY_F6",  VK_F6);
	setKey("KEY_F7",  VK_F7);  setKey("KEY_F8",  VK_F8);
	setKey("KEY_F9",  VK_F9);  setKey("KEY_F10", VK_F10);
	setKey("KEY_F11", VK_F11); setKey("KEY_F12", VK_F12);

	setKey("KEY_TILDE", VK_OEM_3);
	setKey("KEY_PLUS",  VK_OEM_PLUS);
	setKey("KEY_MINUS", VK_OEM_MINUS);
	setKey("KEY_COMMA", VK_OEM_COMMA);
	setKey("KEY_PERIOD",VK_OEM_PERIOD);

	setKey("KEY_UP",    VK_UP);
	setKey("KEY_DOWN",  VK_DOWN);
	setKey("KEY_LEFT",  VK_LEFT);
	setKey("KEY_RIGHT", VK_RIGHT);
	setKey("KEY_INSERT",VK_INSERT);
	setKey("KEY_DELETE",VK_DELETE);
	setKey("KEY_HOME",  VK_HOME);
	setKey("KEY_END",   VK_END);

	setKey("KEY_NUM0", VK_NUMPAD0); setKey("KEY_NUM1", VK_NUMPAD1);
	setKey("KEY_NUM2", VK_NUMPAD2); setKey("KEY_NUM3", VK_NUMPAD3);
	setKey("KEY_NUM4", VK_NUMPAD4); setKey("KEY_NUM5", VK_NUMPAD5);
	setKey("KEY_NUM6", VK_NUMPAD6); setKey("KEY_NUM7", VK_NUMPAD7);
	setKey("KEY_NUM8", VK_NUMPAD8); setKey("KEY_NUM9", VK_NUMPAD9);
	setKey("KEY_MULTIPLY", VK_MULTIPLY);
	setKey("KEY_ADD",      VK_ADD);
	setKey("KEY_SUBTRACT", VK_SUBTRACT);
	setKey("KEY_DECIMAL",  VK_DECIMAL);
	setKey("KEY_DIVIDE",   VK_DIVIDE);

	for (char c = 'A'; c <= 'Z'; c++) {
		char name[8];
		snprintf(name, sizeof(name), "KEY_%c", c);
		setKey(name, c);
	}

	for (char c = '0'; c <= '9'; c++) {
		char name[8];
		snprintf(name, sizeof(name), "KEY_%c", c);
		setKey(name, c);
	}

	setKey("KEY_LSHIFT", VK_LSHIFT);
	setKey("KEY_RSHIFT", VK_RSHIFT);
	setKey("KEY_LCTRL",  VK_LCONTROL);
	setKey("KEY_RCTRL",  VK_RCONTROL);
	setKey("KEY_LALT",   VK_LMENU);
	setKey("KEY_SPACE",  VK_SPACE);
	setKey("KEY_ENTER",  VK_RETURN);
	setKey("KEY_ESC",    VK_ESCAPE);
}

void LuaRegister() {
	L = luaL_newstate();
	luaL_openlibs(L);

	lua_register(L, "FindFirstOf", Lua_FindFirstOf);
	lua_register(L, "FindLastOf",  Lua_FindLastOf);
	lua_register(L, "FindAllOf",   Lua_FindAllOf);
	lua_register(L, "GetName", Lua_GetName);
	lua_register(L, "WriteFloat", Lua_WriteFloat);
	lua_register(L, "ReadFloat", Lua_ReadFloat);
	lua_register(L, "ReadInt", Lua_ReadInt);
	lua_register(L, "RegisterKeyBind", Lua_RegisterKeyBind);
	lua_register(L, "GetGEngine", Lua_GetGEngine);
	lua_register(L, "ReadPtr", Lua_ReadPtr);
	lua_register(L, "GetArrayElement", Lua_GetArrayElement);
	lua_register(L, "Inspect", Lua_Inspect);
	lua_register(L, "RegisterEventHook", Lua_RegisterEventHook);

	luaL_newmetatable(L, "UObjectMeta");
	lua_pushcfunction(L, Lua_ObjectNewIndex);
	lua_setfield(L, -2, "__newindex");
	lua_pushcfunction(L, Lua_ObjectIndex);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	LuaKeyRegister(L);
}

void close_lua() {if(L)lua_close(L);}
