#pragma once

#include <tier1/convar.h>

extern void VProfLoad();
extern void VProfUnload();
extern void VProfConVar(ConVar convar);

extern ICvar* cvar;
extern ConVar vprof_showhooks;
extern void AddLuaHooks();
extern void RemoveLuaHooks();

extern ConVar vprof_exportreport;
extern void AddVProfExport();
extern void RemoveVProfExport();