#pragma once

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <map>

#define IS_VALID(p) (p && (uintptr_t)p > 0x10000 && !IsBadReadPtr((void*)p, 8))

struct UClass;
struct UProperty;

struct FName {
	int32_t ComparisonIndex;
	int32_t Number;
};

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

struct UObject {
	void** vtable;        // 0x00
	int32_t ObjectFlags;  // 0x08
	int32_t InternalIndex;// 0x0C
	UClass* ClassPrivate; // 0x10
	FName NamePrivate;    // 0x18
	UObject* OuterPrivate;// 0x20
};

struct UField : UObject {
	UField* Next;  // 0x28
};

struct UStruct : UField {
	UStruct* SuperStruct;    // 0x30
	UField*  Children;       // 0x38
	int32_t  PropertiesSize; // 0x40
	int32_t  MinAlignment;   // 0x44
	// TArray<uint8> Script   0x48 (16 bytes)
	char     padding_script[0x10];
	void*    PropertyLink;   // 0x58
};

struct UProperty : UField {
	int32_t   ArrayDim;        // 0x30
	int32_t   ElementSize;     // 0x34
	uint64_t  PropertyFlags;   // 0x38
	uint16_t  RepIndex;        // 0x40
	uint8_t   Condition;       // 0x42
	uint8_t   padding_01;      // 0x43
	int32_t   Offset_Internal; // 0x44
	FName     RepNotifyFunc;   // 0x48
	UProperty* PropertyLinkNext; // 0x50
};

struct FProperty {
	char padding_00[0x38];
	int32_t ArrayDim;       // 0x38
	int32_t ElementSize;    // 0x3C
	uint64_t PropertyFlags; // 0x40
	uint16_t RepIndex;      // 0x48
	char padding_01[0x2];   // align
	int32_t Offset_Internal;// 0x4C
	char padding_02[0x8];   // RepNotifyFunc
	FProperty* PropertyLinkNext; // 0x58 (Next)
};

struct UClass : UStruct {
};

struct UFunction : UStruct {
	uint32_t FunctionFlags;
	uint16_t RepOffset;
	uint8_t NumParms;
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
		if (ChunkIndex >= (uint64_t)NumChunks) return nullptr;
		if (!Objects[ChunkIndex]) return nullptr;
		return Objects[ChunkIndex][WithinChunkIndex].Object;
	}
};

typedef void(__thiscall* tFNameToString)(const void*, FString&);
extern tFNameToString FNameToString;
extern TUObjectArray* GObjects;

typedef void(__fastcall* tProcessEvent)(UObject*, UFunction*, void*);
extern tProcessEvent oProcessEvent;

namespace UE422 {
	// UObject
	constexpr uintptr_t UObject_ClassPrivate      = 0x10;
	constexpr uintptr_t UObject_NamePrivate        = 0x18;
	constexpr uintptr_t UField_Next                = 0x28;
	constexpr uintptr_t UStruct_SuperStruct        = 0x40;
	constexpr uintptr_t UStruct_Children           = 0x48;
	constexpr uintptr_t UStruct_PropertyLink       = 0x68;
	constexpr uintptr_t UProperty_Offset_Internal  = 0x44;

	// FField
	constexpr uintptr_t FField_ClassPrivate    = 0x00;
	constexpr uintptr_t FField_NamePrivate     = 0x08;
	constexpr uintptr_t FField_Next            = 0x20;

	// FProperty
	constexpr uintptr_t FProp_ArrayDim         = 0x38;
	constexpr uintptr_t FProp_ElementSize      = 0x3C;
	constexpr uintptr_t FProp_PropertyFlags    = 0x40;
	constexpr uintptr_t FProp_Offset_Internal  = 0x4C;
	constexpr uintptr_t FProp_RepNotifyFunc    = 0x50;
	constexpr uintptr_t FProp_PropertyLinkNext = 0x58;

	// FBoolProperty
	constexpr uintptr_t FBoolProp_FieldSize    = 0x78;
	constexpr uintptr_t FBoolProp_ByteOffset   = 0x79;
	constexpr uintptr_t FBoolProp_ByteMask     = 0x7A;
	constexpr uintptr_t FBoolProp_FieldMask    = 0x7B;

	// FFieldClass
	constexpr uintptr_t FFieldClass_Name       = 0x00;
}
