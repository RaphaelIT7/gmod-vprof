#include "main.h"

void VProfLoad()
{
	if (vprof_showhooks.GetBool())
		AddLuaHooks();

	AddVProfExport();
}

void VProfUnload()
{
	RemoveLuaHooks();
	RemoveVProfExport();
}