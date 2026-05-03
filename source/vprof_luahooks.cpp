#include <GarrysMod/InterfacePointers.hpp>
#include <GarrysMod/Lua/LuaShared.h>
#include <GarrysMod/FactoryLoader.hpp>
#include <GarrysMod/Lua/LuaInterface.h>
#include "main.h"
#include "detours.h"
#include <memory>
#include <vprof.h>
#include <map>
#include <string>

// Minimal lua_Debug definition matching GarrysMod's LuaJIT (LUA_IDSIZE = 128)
struct lua_Debug {
	int event;
	const char* name;
	const char* namewhat;
	const char* what;
	const char* source;
	int currentline;
	int nups;
	int linedefined;
	int lastlinedefined;
	char short_src[128];
	int i_ci;
};

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
Detouring::Hook detour_CLuaInterface_CallFunctionProtected;

GarrysMod::Lua::CLuaInterface* gLUA;
void CheckLua()
{
	if (!gLUA)
	{
		SourceSDK::FactoryLoader lua_shared_loader("lua_shared");
		GarrysMod::Lua::ILuaShared* shared = lua_shared_loader.GetInterface<GarrysMod::Lua::ILuaShared>(GMOD_LUASHARED_INTERFACE);
		gLUA = (GarrysMod::Lua::CLuaInterface*)shared->GetLuaInterface(GarrysMod::Lua::State::SERVER);

		if (!gLUA)
		{
			Msg("Failed to get ILuaInterface!. Were about to crash with this one\n");
		}
	}
}

#ifdef SYSTEM_WINDOWS
std::map<int, std::string> Call_strs;
std::map<std::string, std::string> CallFunctionProtected_ids;  // key -> short id
std::map<std::string, std::string> CallFunctionProtected_labels; // short id -> full label
static int cfp_counter = 0;
class CLuaInterfaceProxy : public Detouring::ClassProxy<GarrysMod::Lua::CLuaInterface, CLuaInterfaceProxy> {
public:
	CLuaInterfaceProxy(GarrysMod::Lua::CLuaInterface* LUA) {
		if (CheckValue("initialize", "CLuaInterfaceProxy", Initialize(LUA)))
		{
			CheckValue("CLuaInterface::PushPooledString", Hook(&GarrysMod::Lua::CLuaInterface::PushPooledString, &CLuaInterfaceProxy::PushPooledString));
			CheckValue("CLuaInterface::CallFunctionProtected", Hook(&GarrysMod::Lua::CLuaInterface::CallFunctionProtected, &CLuaInterfaceProxy::CallFunctionProtected));
		}
	}

	void DeInit()
	{
		UnHook(&GarrysMod::Lua::CLuaInterface::PushPooledString);
		UnHook(&GarrysMod::Lua::CLuaInterface::CallFunctionProtected);
	}

	virtual void GMCOMMON_CALLING_CONVENTION PushPooledString(int pool)
	{
		if (Call_strs.find(pool) == Call_strs.end())
		{
			Call_strs[pool] = "CLuaInterface::PushPooledString (" + (std::string)gLUA->GetPooledString(pool) + ")";
		}

		VPROF_BUDGET( Call_strs[pool].c_str(), "GMOD" );

		Call(&GarrysMod::Lua::CLuaInterface::PushPooledString, pool);
	}

	virtual bool GMCOMMON_CALLING_CONVENTION CallFunctionProtected(int iArgs, int iRets, bool bShowErrors)
	{
		lua_Debug ar = {};
		Push(-(iArgs + 1));
		GetInfo(">Sn", &ar);

		std::string key(ar.short_src);
		key += ":";
		key += std::to_string(ar.linedefined);

		auto it = CallFunctionProtected_ids.find(key);
		if (it == CallFunctionProtected_ids.end())
		{
			const char* src = ar.short_src[0] ? ar.short_src : "?";
			if (strncmp(src, "addons/", 7) == 0)
				src += 7;
			char id[16];
			snprintf(id, sizeof(id), "LUA#%04d", cfp_counter++);
			std::string full = "CLuaInterface::CallFunctionProtected(";
			full += (ar.linedefined > 0) ? std::to_string(ar.linedefined) : "?";
			full += "@";
			full += src;
			full += ")";
			CallFunctionProtected_ids[key] = id;
			CallFunctionProtected_labels[id] = std::move(full);
			it = CallFunctionProtected_ids.find(key);
		}

		VPROF_BUDGET( it->second.c_str(), "GMOD" );

		return Call(&GarrysMod::Lua::CLuaInterface::CallFunctionProtected, iArgs, iRets, bShowErrors);
	}

	static std::unique_ptr<CLuaInterfaceProxy> Singleton;
};
std::unique_ptr<CLuaInterfaceProxy> CLuaInterfaceProxy::Singleton;
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

std::map<std::string, std::string> CallFunctionProtected_ids;  // key -> short id
std::map<std::string, std::string> CallFunctionProtected_labels; // short id -> full label
static int cfp_counter = 0;
bool hook_CLuaInterface_CallFunctionProtected(void* self, int iArgs, int iRets, bool bShowError)
{
	CheckLua();

	lua_Debug ar = {};
	gLUA->Push(-(iArgs + 1));
	gLUA->GetInfo(">Sn", &ar);

	std::string key(ar.short_src);
	key += ":";
	key += std::to_string(ar.linedefined);

	auto it = CallFunctionProtected_ids.find(key);
	if (it == CallFunctionProtected_ids.end())
	{
		const char* src = ar.short_src[0] ? ar.short_src : "?";
		if (strncmp(src, "addons/", 7) == 0)
			src += 7;
		char id[16];
		snprintf(id, sizeof(id), "LUA#%04d", cfp_counter++);
		std::string full = "CLuaInterface::CallFunctionProtected(";
		full += src;
		full += ":";
		full += (ar.linedefined > 0) ? std::to_string(ar.linedefined) : "?";
		full += ")";
		CallFunctionProtected_ids[key] = id;
		CallFunctionProtected_labels[id] = std::move(full);
		it = CallFunctionProtected_ids.find(key);
	}

	VPROF_BUDGET( it->second.c_str(), "GMOD" );

	return detour_CLuaInterface_CallFunctionProtected.GetTrampoline<CLuaInterface_CallFunctionProtected>()(self, iArgs, iRets, bShowError);
}
#endif

bool bActiveLuaHooks;
void AddLuaHooks()
{
	RemoveLuaHooks();

#ifdef SYSTEM_WINDOWS
	SourceSDK::ModuleLoader lua_shared_loader("lua_shared");
	CheckLua();
	CLuaInterfaceProxy::Singleton = std::make_unique<CLuaInterfaceProxy>(gLUA);
#else
	SourceSDK::ModuleLoader server_loader("server");
	CreateDetour(&detour_CLuaGamemode_CallFinish, "CLuaGamemode::CallFinish", server_loader.GetModule(), CLuaGamemode_CallFinishSym, (void*)hook_CLuaGamemode_CallFinish, DETOUR_LUAHOOKS);
	CreateDetour(&detour_CLuaGamemode_CallWithArgs, "CLuaGamemode::CallWithArgs", server_loader.GetModule(), CLuaGamemode_CallWithArgsSym, (void*)hook_CLuaGamemode_CallWithArgs, DETOUR_LUAHOOKS);
	CreateDetour(&detour_CLuaGamemode_Call, "CLuaGamemode::Call", server_loader.GetModule(), CLuaGamemode_CallSym, (void*)hook_CLuaGamemode_Call, DETOUR_LUAHOOKS);
	SourceSDK::ModuleLoader lua_shared_loader("lua_shared");
	CreateDetour(&detour_CLuaInterface_CallFunctionProtected, "CLuaInterface::CallFunctionProtected", lua_shared_loader.GetModule(), CLuaInterface_CallFunctionProtectedSym, (void*)hook_CLuaInterface_CallFunctionProtected, DETOUR_LUAHOOKS);
#endif
}

void RemoveLuaHooks()
{
#ifdef SYSTEM_WINDOWS
	CLuaInterfaceProxy::Singleton->DeInit();
	CLuaInterfaceProxy::Singleton.reset();
#else
	RemoveDetours(DETOUR_LUAHOOKS);
	gLUA = nullptr;
#endif
}