#pragma once
#include <windows.h>
#include <vector>
#include <Psapi.h>

namespace Scanner {
	inline uintptr_t FindPattern(const char* signature) {
		static auto pattern_to_byte = [](const char* pattern) {
			auto bytes = std::vector<int>{};
			auto start = const_cast<char*>(pattern);
			auto end = const_cast<char*>(pattern) + strlen(pattern);
			for (auto curr = start; curr < end; ++curr) {
				if (*curr == '?') {
					++curr;
					if (*curr == '?') ++curr;
					bytes.push_back(-1);
				}
				else {
					bytes.push_back(strtoul(curr, &curr, 16));
				}
			}
			return bytes;
			};

		auto module = GetModuleHandleA(NULL);
		MODULEINFO moduleInfo;
		GetModuleInformation(GetCurrentProcess(), module, &moduleInfo, sizeof(MODULEINFO));

		auto patternBytes = pattern_to_byte(signature);
		auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

		auto s = patternBytes.size();
		auto d = patternBytes.data();

		for (auto i = 0ul; i < moduleInfo.SizeOfImage - s; ++i) {
			bool found = true;
			for (auto j = 0ul; j < s; ++j) {
				if (scanBytes[i + j] != d[j] && d[j] != -1) {
					found = false;
					break;
				}
			}
			if (found) return reinterpret_cast<uintptr_t>(&scanBytes[i]);
		}
		return 0;
	}
}
