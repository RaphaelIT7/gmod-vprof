#include <GarrysMod/Lua/Interface.h>
#include "main.h"

GMOD_MODULE_OPEN()
{
	VProfLoad();
	return 0;
}

GMOD_MODULE_CLOSE()
{
	VProfUnload();
	return 0;
}