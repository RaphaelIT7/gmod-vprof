#include <GarrysMod/InterfacePointers.hpp>
#include "main.h"

void VProfLoad()
{
	g_pCVar = InterfacePointers::Cvar();

	if (vprof_showhooks.GetBool())
		AddLuaHooks();

	AddVProfExport();

	ConVar_Register();
}

void VProfUnload()
{
	RemoveLuaHooks();
	RemoveVProfExport();

	ConVar_Unregister();
}