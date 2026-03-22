#include <windows.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <map>
#include <string>
#include "Scanner.h"
#include "SDK.h"
#include "Utils.h"

extern "C" {
	#include "lua/lua.h"
	#include "lua/lualib.h"
	#include "lua/lauxlib.h"
}

namespace fs = std::filesystem;

bool IsGameFullyLoaded() {
	if (!GObjects) return false;
	for (int i = 0; i < GObjects->NumElements; i++) {
		UObject* obj = GObjects->GetObjectById(i);
		if (!obj || (uintptr_t)obj < 0x10000) continue;
		std::string name = GetName(obj);
		if (name.find("UMG_TitleScreen") != std::string::npos &&
			name.find("Default__") == std::string::npos)
			return true;
	}
	return false;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
	AllocConsole();
	FILE* f; freopen_s(&f, "CONOUT$", "w", stdout);

	std::cout << "[Tavern] Initializing" << std::endl;

	uintptr_t addrTS = Scanner::FindPattern("48 89 5C 24 08 57 48 83 EC 30 83 79 04 00");
	if (!addrTS) {
		std::cout << "[Tavern] Error: FNameToString not found" << std::endl;
		FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), 0);
		return 0;
	}
	FNameToString = (tFNameToString)addrTS;

	uintptr_t addrGObj = Scanner::FindPattern("48 8B 05 ? ? ? ? 48 8B 0C C8 48 8B 04 D1");
	if (!addrGObj) {
		std::cout << "[Tavern] Error: GObjects not found" << std::endl;
		FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), 0);
		return 0;
	}
	int32_t relOffset = *(int32_t*)(addrGObj + 3);
	GObjects = (TUObjectArray*)(addrGObj + 7 + relOffset);

	std::cout << "[Tavern] Waiting for the game ..." << std::endl;
	while (!IsGameFullyLoaded()) Sleep(500);
	std::cout << "[Tavern] Game ready (" << std::dec << GObjects->NumElements << " objects)" << std::endl;

	LuaRegister();
	std::cout << "[Tavern] Lua engine ready" << std::endl;

	LoadLuaMods();
	std::cout << "[Tavern] Mod loader ready" << std::endl;

	while (!(GetAsyncKeyState(VK_END) & 1)) {
		CheckLuaInputs();
		Sleep(10);
	}

	close_lua();
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
