#pragma once

#include <tier1/convar.h>
#include <string>

extern void VProfLoad();
extern void VProfUnload();

extern ConVar vprof_showhooks;
extern void AddLuaHooks();
extern void RemoveLuaHooks();

extern ConVar vprof_exportreport;
extern void AddVProfExport();
extern void RemoveVProfExport();

// Brought over from HolyLib
struct StringHash {
	using is_transparent = void;
	size_t operator()(std::string_view sv) const noexcept {
		return std::hash<std::string_view>{}(sv);
	}
};
struct StringEq {
	using is_transparent = void;
	bool operator()(std::string_view a, std::string_view b) const noexcept {
		return a == b;
	}
};