#include <GarrysMod/InterfacePointers.hpp>
#include <GarrysMod/Lua/LuaInterface.h>
#include <GarrysMod/Lua/LuaShared.h>
#include <GarrysMod/FactoryLoader.hpp>
#include "main.h"
#include "detours.h"
#include <vprof.h>
#include <map>

static void OnVProfShowHooksChange(IConVar* var, const char *pOldValue, float flOldValue)
{
	if (vprof_showhooks.GetBool()) {
		AddLuaHooks();
	} else {
		RemoveLuaHooks();
	}
}
ConVar vprof_showhooks("vprof_showhooks", "1", 0, "If enabled, vprof will show the name of all called hooks", OnVProfShowHooksChange);

Detouring::Hook detour_CLuaGamemode_CallFinish;
Detouring::Hook detour_CLuaGamemode_CallWithArgs;
Detouring::Hook detour_CLuaGamemode_Call;
Detouring::Hook detour_CLuaInterface_PushPooledString;

GarrysMod::Lua::ILuaInterface* gLUA;
void CheckLua()
{
	if (!gLUA)
	{
		SourceSDK::FactoryLoader lua_shared_loader("lua_shared");
		GarrysMod::Lua::ILuaShared* shared = lua_shared_loader.GetInterface<GarrysMod::Lua::ILuaShared>(GMOD_LUASHARED_INTERFACE);
		gLUA = shared->GetLuaInterface(GarrysMod::Lua::State::SERVER);

		if (!gLUA)
		{
			Msg("Failed to get ILuaInterface!. Were about to crash with this one\n");
		}
	}
}

#ifdef SYSTEM_WINDOWS
std::map<int, std::string> Call_strs;
void hook_CLuaInterface_PushPooledString(int pool) // BUG: Why does this crash?
{
	CheckLua();

	if (Call_strs.find(pool) == Call_strs.end())
	{
		Call_strs[pool] = "CLuaInterface::PushPooledString (" + (std::string)gLUA->GetPooledString(pool) + ")";
	}

	VPROF_BUDGET( Call_strs[pool].c_str(), "GMOD" );

	detour_CLuaInterface_PushPooledString.GetTrampoline<CLuaInterface_PushPooledString>()((void*)gLUA, pool);
}
#else
std::map<int, std::string> CallFinish_strs;
void* hook_CLuaGamemode_CallFinish(void* funky_srv, int pool)
{
	CheckLua();
	if (CallFinish_strs.find(pool) == CallFinish_strs.end())
	{
		CallFinish_strs[pool] = "CLuaGamemode::CallFinish (" + (std::string)gLUA->GetPooledString(pool) + ")";
	}

	VPROF_BUDGET( CallFinish_strs[pool].c_str(), "GMOD" );

	return detour_CLuaGamemode_CallFinish.GetTrampoline<CLuaGamemode_CallFinish>()(funky_srv, pool);
}

std::map<int, std::string> CallWithArgs_strs;
void* hook_CLuaGamemode_CallWithArgs(void* funky_srv, int pool)
{
	CheckLua();
	if (CallWithArgs_strs.find(pool) == CallWithArgs_strs.end())
	{
		CallWithArgs_strs[pool] = "CLuaGamemode::CallWithArgs (" + (std::string)gLUA->GetPooledString(pool) + ")";
	}

	VPROF_BUDGET( CallWithArgs_strs[pool].c_str(), "GMOD" );

	return detour_CLuaGamemode_CallWithArgs.GetTrampoline<CLuaGamemode_CallWithArgs>()(funky_srv, pool);
}

std::map<int, std::string> Call_strs;
void* hook_CLuaGamemode_Call(void* funky_srv, int pool)
{
	CheckLua();
	if (Call_strs.find(pool) == Call_strs.end())
	{
		Call_strs[pool] = "CLuaGamemode::Call (" + (std::string)gLUA->GetPooledString(pool) + ")";
	}

	VPROF_BUDGET( Call_strs[pool].c_str(), "GMOD" );

	return detour_CLuaGamemode_Call.GetTrampoline<CLuaGamemode_Call>()(funky_srv, pool);
}
#endif

bool bActiveLuaHooks;
void AddLuaHooks()
{
	RemoveLuaHooks();

#ifdef SYSTEM_WINDOWS
	SourceSDK::ModuleLoader lua_shared_loader("lua_shared");
	CreateDetour(&detour_CLuaInterface_PushPooledString, "CLuaInterface::PushPooledString", lua_shared_loader.GetModule(), CLuaInterface_PushPooledStringSym, hook_CLuaInterface_PushPooledString, DETOUR_LUAHOOKS);
#else
	SourceSDK::ModuleLoader server_loader("server");
	CreateDetour(&detour_CLuaGamemode_CallFinish, "CLuaGamemode::CallFinish", server_loader.GetModule(), CLuaGamemode_CallFinishSym, (void*)hook_CLuaGamemode_CallFinish, DETOUR_LUAHOOKS);
	CreateDetour(&detour_CLuaGamemode_CallWithArgs, "CLuaGamemode::CallWithArgs", server_loader.GetModule(), CLuaGamemode_CallWithArgsSym, (void*)hook_CLuaGamemode_CallWithArgs, DETOUR_LUAHOOKS);
	CreateDetour(&detour_CLuaGamemode_Call, "CLuaGamemode::Call", server_loader.GetModule(), CLuaGamemode_CallSym, (void*)hook_CLuaGamemode_Call, DETOUR_LUAHOOKS);
#endif
}

void RemoveLuaHooks()
{
	RemoveDetours(DETOUR_LUAHOOKS);
}