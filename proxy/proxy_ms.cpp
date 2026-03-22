#include <windows.h>

#pragma comment(linker, "/export:XInputGetState=C:\\Windows\\System32\\xinput1_4.XInputGetState")
#pragma comment(linker, "/export:XInputSetState=C:\\Windows\\System32\\xinput1_4.XInputSetState")
#pragma comment(linker, "/export:XInputGetCaps=C:\\Windows\\System32\\xinput1_4.XInputGetCaps")
#pragma comment(linker, "/export:XInputGetBatteryInformation=C:\\Windows\\System32\\xinput1_4.XInputGetBatteryInformation")
#pragma comment(linker, "/export:XInputGetKeystroke=C:\\Windows\\System32\\xinput1_4.XInputGetKeystroke")
#pragma comment(linker, "/export:XInputGetAudioDeviceIds=C:\\Windows\\System32\\xinput1_4.XInputGetAudioDeviceIds")
#pragma comment(linker, "/export:XInputEnable=C:\\Windows\\System32\\xinput1_4.XInputEnable")

DWORD WINAPI LoadTavern(LPVOID lpParam) {
	Sleep(3000);
	LoadLibraryA("TavernModLoader.dll");
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul, LPVOID lp) {
	if (ul == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		CreateThread(nullptr, 0, LoadTavern, nullptr, 0, nullptr);
	}
	return TRUE;
}
