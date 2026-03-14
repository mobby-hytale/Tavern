#include <windows.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <map>
#include <string>
#include "Scanner.h"
#include "minhook/MinHook.h"
#include "SDK.h"

extern "C" {
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
}

namespace fs = std::filesystem;

tProcessEvent oProcessEvent = nullptr;
tFNameToString FNameToString = nullptr;
TUObjectArray* GObjects = nullptr;

lua_State* L = nullptr;
std::map<int, int> keyBinds;

int Lua_FindObject(lua_State* L) {
	const char* targetName = luaL_checkstring(L, 1);
	if (!targetName || !GObjects) return 0;

	std::string target(targetName);
	std::transform(target.begin(), target.end(), target.begin(), ::tolower);

	for (int i = 0; i < GObjects->NumElements; i++) {
		UObject* obj = GObjects->GetObjectById(i);
		if (!obj || (uintptr_t)obj < 0x10000) continue;

		std::string name = GetName(obj);
		std::string nameLower = name;
		std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

		if (nameLower.find(target) != std::string::npos) {
			if (name.find("Default__") == std::string::npos) {
				std::cout << "[Tavern] INSTANCE RECENTE TROUVEE : " << name << " (ID: " << i << ")" << std::endl;
				lua_pushlightuserdata(L, obj);
				return 1;
			}
		}
	}
	return 0;
}

int Lua_GetName(lua_State* L) {
	UObject* obj = (UObject*)lua_touserdata(L, 1);
	if (!obj || (uintptr_t)obj < 0x10000) {
		lua_pushstring(L, "InvalidObject");
		return 1;
	}

	FString fs;
	FNameToString((void*)((uintptr_t)obj + 0x18), fs);

	if (fs.Data && fs.Count > 0) {
		std::wstring ws(fs.Data);
		std::string name(ws.begin(), ws.end());
		lua_pushstring(L, name.c_str());
	}
	else {
		lua_pushstring(L, "None");
	}
	return 1;
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
	const char* scriptPath = "Mods\\SpeedMod\\main.lua";

	std::cout << "[Tavern] Chargement des mods : " << scriptPath << std::endl;

	if (luaL_dofile(L, scriptPath) != LUA_OK) {
		std::cout << "[Lua Error] " << lua_tostring(L, -1) << std::endl;
		lua_pop(L, 1);
	}
	else {
		std::cout << "[Tavern] Mod charge" << std::endl;
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

void __fastcall hkProcessEvent(UObject* pObject, UFunction* pFunction, void* pParams) {
	static bool firstCall = true;
	if (firstCall) {
		Beep(1500, 200);
		std::cout << "[Tavern] HOOK ACTIF !" << std::endl;
		firstCall = false;
	}
	oProcessEvent(pObject, pFunction, pParams);
}

void InitializeModLoader() {
	Beep(500, 300);
	std::cout << "[Tavern] Initialisation" << std::endl;

	uintptr_t addrTS = Scanner::FindPattern("48 89 5C 24 08 57 48 83 EC 30 83 79 04 00");
	if (addrTS) FNameToString = (tFNameToString)addrTS;

	uintptr_t addrGE = Scanner::FindPattern("48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 74 05");
	if (addrGE) {
		int32_t offset = *(int32_t*)(addrGE + 3);
		uintptr_t* pGEngine = *(uintptr_t**)(addrGE + 7 + offset);
		if (pGEngine) {
			uintptr_t realProcessEvent = (*(uintptr_t**)pGEngine)[64];
			if (MH_Initialize() == MH_OK) {
				MH_CreateHook((LPVOID)realProcessEvent, (LPVOID)&hkProcessEvent, (LPVOID*)&oProcessEvent);
				MH_EnableHook((LPVOID)realProcessEvent);
			}
		}
	}

	uintptr_t addrGObj = Scanner::FindPattern("48 8B 05 ? ? ? ? 48 8B 0C C8 48 8B 04 D1");
	if (addrGObj) {
		int32_t offset = *(int32_t*)(addrGObj + 3);

		GObjects = (TUObjectArray*)(addrGObj + 7 + offset);

		std::cout << "[Tavern] GObjects trouve a : 0x" << std::hex << GObjects << std::endl;
		std::cout << "[Tavern] Nombre d'objets : " << std::dec << GObjects->NumElements << std::endl;
	}

	L = luaL_newstate();
	luaL_openlibs(L);

	lua_register(L, "FindObject", Lua_FindObject);
	lua_register(L, "GetName", Lua_GetName);
	lua_register(L, "WriteFloat", Lua_WriteFloat);
	lua_register(L, "RegisterKeyBind", Lua_RegisterKeyBind);
	lua_register(L, "GetGEngine", Lua_GetGEngine);
	lua_register(L, "ReadPtr", Lua_ReadPtr);
	lua_register(L, "GetArrayElement", Lua_GetArrayElement);

	lua_pushinteger(L, VK_F1); lua_setglobal(L, "KEY_F1");
	lua_pushinteger(L, VK_F2); lua_setglobal(L, "KEY_F2");
	lua_pushinteger(L, VK_F3); lua_setglobal(L, "KEY_F3");

	std::cout << "[Tavern] Moteur Lua pret" << std::endl;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
	AllocConsole();
	FILE* f; freopen_s(&f, "CONOUT$", "w", stdout);

	InitializeModLoader();
	LoadLuaMods();

	std::cout << "[Tavern] Loader" << std::endl;

	while (!(GetAsyncKeyState(VK_END) & 1)) {
		CheckLuaInputs();
		Sleep(10);
	}

	if (L) lua_close(L);
	if (f) fclose(f);
	FreeConsole();
	FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul, LPVOID lp) {
	if (ul == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
	}
	return TRUE;
}
