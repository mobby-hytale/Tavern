#include "SDK.h"

std::string GetName(UObject* Obj) {
	if (!Obj) return "NULL";
	FString FS;
	FNameToString((void*)((uintptr_t)Obj + 0x18), FS);
	return FS.ToString();
}

UObject* FindObject(std::string NameContains) {
	if (!GObjects) return nullptr;
	for (int i = 0; i < GObjects->NumElements; i++) {
		UObject* Obj = GObjects->GetObjectById(i);
		if (!Obj) continue;

		if (GetName(Obj).find(NameContains) != std::string::npos) {
			return Obj;
		}
	}
	return nullptr;
}

UObject* GetPlayerCharacter() {
	UObject* found = FindObject("BP_DungeonsPlayerCharacter");
	if (!found) found = FindObject("DungeonsPlayerCharacter");
	if (!found) found = FindObject("PlayerCharacter");
	if (!found) found = FindObject("Pawn");

	if (found) {
		std::cout << "[Tavern] Joueur detecte sous le nom : " << GetName(found) << std::endl;
	}
	return found;
}
