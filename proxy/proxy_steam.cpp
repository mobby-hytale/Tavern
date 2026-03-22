#include <windows.h>

#pragma comment(linker, "/export:XInputGetState=C:\\Windows\\System32\\xinput1_3.XInputGetState")
#pragma comment(linker, "/export:XInputSetState=C:\\Windows\\System32\\xinput1_3.XInputSetState")
#pragma comment(linker, "/export:XInputGetCaps=C:\\Windows\\System32\\xinput1_3.XInputGetCaps")
#pragma comment(linker, "/export:XInputGetDSoundAudioDeviceGuids=C:\\Windows\\System32\\xinput1_3.XInputGetDSoundAudioDeviceGuids")

DWORD WINAPI LoadTavern(LPVOID lpParam) {
	Sleep(3000);
	LoadLibraryA("TavernModLoader.dll");
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, 0, LoadTavern, NULL, 0, NULL);
	}
	return TRUE;
}
