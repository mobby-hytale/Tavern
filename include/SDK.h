#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

struct UObject { void** vtable; };
struct UFunction {};

struct FString {
	wchar_t* Data;
	int Count;
	int Max;
	std::string ToString() const {
		if (!Data || Count <= 0) return "";
		std::wstring ws(Data);
		return std::string(ws.begin(), ws.end());
	}
};

struct FUObjectItem {
	UObject* Object;
	int32_t Flags;
	int32_t ClusterRootIndex;
	int32_t SerialNumber;
};

struct TUObjectArray {
	FUObjectItem** Objects;
	FUObjectItem* PreAllocatedObjects;
	int32_t MaxElements;
	int32_t NumElements;
	int32_t MaxChunks;
	int32_t NumChunks;

	UObject* GetObjectById(int32_t Index) {
		if (Index >= NumElements || Index < 0) return nullptr;
		uint64_t ChunkIndex = Index / 65536;
		uint64_t WithinChunkIndex = Index % 65536;
		if (ChunkIndex >= NumChunks) return nullptr;
		return Objects[ChunkIndex][WithinChunkIndex].Object;
	}
};


typedef void(__thiscall* tFNameToString)(const void*, FString&);
extern tFNameToString FNameToString;
extern TUObjectArray* GObjects;

std::string GetName(UObject* Obj);
UObject* FindObject(std::string NameContains);
UObject* GetPlayerCharacter();

typedef void(__thiscall* tProcessEvent)(UObject*, UFunction*, void*);

extern tProcessEvent oProcessEvent;
