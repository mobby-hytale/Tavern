#include "SDK.h"

tProcessEvent oProcessEvent = nullptr;
tFNameToString FNameToString = nullptr;
TUObjectArray* GObjects = nullptr;

std::string ReadUEName(uintptr_t objectBase) {
	__try {
		FString fs;
		FNameToString((void*)(objectBase + UE422::UObject_NamePrivate), fs);
		std::string name = fs.ToString();
		if (!name.empty() && name.length() < 128)
			return name;
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	return "";
}

std::string GetName(UObject* Obj) {
	if (!IS_VALID(Obj)) return "INVALID_PTR";
	std::string name = ReadUEName((uintptr_t)Obj);
	return name.empty() ? "None" : name;
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

bool IsPropertyType(const std::string& typeName) {
	return typeName.find("Property") != std::string::npos;
}

uintptr_t FindPropertyOffset(UObject* Obj, const std::string& propName) {
	if (!Obj || propName.empty()) return 0;

	uintptr_t CurrentStruct = (uintptr_t)Obj->ClassPrivate;
	int depth = 0;

	while (IS_VALID(CurrentStruct) && depth < 16) {
		depth++;

		uintptr_t Prop = *(uintptr_t*)(CurrentStruct + UE422::UStruct_Children);
		int count = 0;

		while (IS_VALID(Prop) && count < 256) {
			count++;

			std::string name = ReadUEName(Prop);

			if (name == propName) {
				uintptr_t fieldClassPtr = *(uintptr_t*)(Prop + UE422::UObject_ClassPrivate);
				if (IS_VALID(fieldClassPtr)) {
					std::string typeName = ReadUEName(fieldClassPtr);
					if (IsPropertyType(typeName)) {
						int32_t offset = *(int32_t*)(Prop + UE422::UProperty_Offset_Internal);
						return (uintptr_t)offset;
					}
				}
			}

			Prop = *(uintptr_t*)(Prop + UE422::UField_Next);
		}

		uintptr_t Super = *(uintptr_t*)(CurrentStruct + UE422::UStruct_SuperStruct);
		if (!IS_VALID(Super) || Super == CurrentStruct) break;
		if (ReadUEName(Super) == "Object") break;
		CurrentStruct = Super;
	}

	return 0;
}
