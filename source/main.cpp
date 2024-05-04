#include <GarrysMod/InterfacePointers.hpp>
#include "main.h"

void VProfLoad()
{
	cvar = InterfacePointers::Cvar();

	if (vprof_showhooks.GetBool())
		AddLuaHooks();

	AddVProfExport();

	VProfConVar(vprof_exportreport);
	VProfConVar(vprof_showhooks);
}

void VProfUnload()
{
	RemoveLuaHooks();
	RemoveVProfExport();
}

void VProfConVar(ConVar convar)
{
	if (!convar.IsRegistered())
	{
		//cvar->RegisterConCommand((ConCommandBase*)&convar);
	}
}