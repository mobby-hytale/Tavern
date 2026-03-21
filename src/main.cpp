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

namespace fs = std::filesystem;

void __fastcall hkProcessEvent(UObject* pObject, UFunction* pFunction, void* pParams) {
	static bool firstCall = true;
	if (firstCall) {
		std::cout << "[Tavern] Hook active" << std::endl;
		firstCall = false;
	}
	oProcessEvent(pObject, pFunction, pParams);
}

void InitializeModLoader() {
	std::cout << "[Tavern] Initializing" << std::endl;

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

		std::cout << "[Tavern] GObjects found at: 0x" << std::hex << GObjects << std::endl;
		std::cout << "[Tavern] Object count: " << std::dec << GObjects->NumElements << std::endl;
	}

	LuaRegister();

	std::cout << "[Tavern] Lua engine ready" << std::endl;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
	AllocConsole();
	FILE* f; freopen_s(&f, "CONOUT$", "w", stdout);

	InitializeModLoader();
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
