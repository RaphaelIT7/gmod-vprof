#pragma once

#include <tier1/convar.h>

extern void VProfLoad();
extern void VProfUnload();

extern ConVar vprof_showhooks;
extern void AddLuaHooks();
extern void RemoveLuaHooks();

extern void AddVProfExport();
extern void RemoveVProfExport();