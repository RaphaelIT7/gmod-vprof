#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/ModuleLoader.hpp>
#include <scanning/symbolfinder.hpp>
#include <GarrysMod/Symbol.hpp>
#include <detouring/classproxy.hpp>
#include <detouring/hook.hpp>

inline bool CheckValue(const char* msg, const char* name, bool ret)
{
	if (!ret) {
		Msg("[vprof] Failed to %s %s!\n", msg, name);
		return false;
	}

	return true;
}

inline bool CheckValue(const char* name, bool ret)
{
	return CheckValue("get function", name, ret);
}

template<class T>
bool CheckFunction(T func, const char* name)
{
	return CheckValue("get function", name, func != nullptr);
}

enum DETOUR_CATEGOY {
	DETOUR_ALL,
	DETOUR_LUAHOOKS,
	DETOUR_VPROFEXPORT,
};

extern void* GetFunction(void* module, Symbol symbol);
extern void CreateDetour(Detouring::Hook* hook, const char* name, void* module, Symbol symbol, void* func, DETOUR_CATEGOY category);
extern void RemoveDetours(DETOUR_CATEGOY category);

#if defined SYSTEM_WINDOWS
#if defined ARCHITECTURE_X86_64
#define GMCOMMON_CALLING_CONVENTION __fastcall
#else
#define GMCOMMON_CALLING_CONVENTION __thiscall
#endif
#else
#define GMCOMMON_CALLING_CONVENTION
#endif

typedef void (GMCOMMON_CALLING_CONVENTION* CLuaInterface_PushPooledString)(void* srv, int pool);
typedef void* (GMCOMMON_CALLING_CONVENTION* CLuaGamemode_CallFinish)(void* srv, int pool);
typedef void* (GMCOMMON_CALLING_CONVENTION* CLuaGamemode_CallWithArgs)(void* srv, int pool);
typedef void* (GMCOMMON_CALLING_CONVENTION* CLuaGamemode_Call)(void* srv, int pool);
typedef void (GMCOMMON_CALLING_CONVENTION* CVProfile_OutputReport)(void* srv, int, const tchar*, int);

#ifdef SYSTEM_WINDOWS
#ifdef ARCHITECTURE_X86
// ToDo: Find out how to get gGM and detour CLuaGamemode without breaking it.
const Symbol CVProfile_OutputReportSym = Symbol::FromName("?OutputReport@CVProfile@@QAEXHPBDH@Z"); //Symbol::FromSignature("\x55\x8B\xEC\x83\xEC\x08\x56\x68\xFC\xFE**\x8B\xF1"); // 55 8B EC 83 EC 08 56 68 FC FE ?? ?? 8B F1
const Symbol CLuaInterface_PushPooledStringSym = Symbol::FromSignature("\x55\x8B\xEC\x56\x8B\xF1\x8B\x8E\xE8"); // 55 8B EC 56 8B F1 8B 8E E8
//const Symbol CLuaGamemode_CallFinishSym = Symbol::FromSignature("\x55\x8B\xEC\x8B\x0D\xD4***\x53\x56"); // 55 8B EC 8B 0D D4 ?? ?? ?? 53 56
//const Symbol CLuaGamemode_CallWithArgsSym = Symbol::FromSignature("\x55\x8B\xEC\x56\x8B\xF1\x8B\x0D\xD4***\x57\x8B\xB9\x0C***\x85\xFF**\x6A\x04\x6A\x00\x68\x48\xA8**\x6A\x00\x68\x40\xAB**\xFF\x15\xDC***\xFF*****\x84\xC0\x75\x0E"); // 55 8B EC 56 8B F1 8B 0D D4 ?? ?? ?? 57 8B B9 0C ?? ?? ?? 85 FF ?? ?? 6A 04 6A 00 68 48 A8 ?? ?? 6A 00 68 40 AB ?? ?? FF 15 DC ?? ?? ?? FF ?? ?? ?? ?? ?? 84 C0 75 0E
//const Symbol CLuaGamemode_CallSym = Symbol::FromSignature("\x55\x8B\xEC\x56\x8B\xF1\x8B\x0D\xD4***\x57\x8B\xB9\x0C***\x85\xFF************************************************\x8B\x06"); // 55 8B EC 56 8B F1 8B 0D D4 ?? ?? ?? 57 8B B9 0C ?? ?? ?? 85 FF ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 8B 06 8B CE 8B 80 8C ?? ?? ?? FF D0
#else
const Symbol CLuaGamemode_CallFinishSym = Symbol::FromSignature("");
const Symbol CLuaGamemode_CallWithArgsSym = Symbol::FromSignature("");
const Symbol CLuaGamemode_CallSym = Symbol::FromSignature("");
#endif
#else
#ifdef ARCHITECTURE_X86
const Symbol CLuaGamemode_CallFinishSym = Symbol::FromName("_ZN12CLuaGamemode10CallFinishEi");
const Symbol CLuaGamemode_CallWithArgsSym = Symbol::FromName("_ZN12CLuaGamemode12CallWithArgsEi");
const Symbol CLuaGamemode_CallSym = Symbol::FromName("_ZN12CLuaGamemode4CallEi");
const Symbol CVProfile_OutputReportSym = Symbol::FromName("_ZN9CVProfile12OutputReportEiPKci");
#else
const Symbol CLuaGamemode_CallFinishSym = Symbol::FromSignature("");
const Symbol CLuaGamemode_CallWithArgsSym = Symbol::FromSignature("");
const Symbol CLuaGamemode_CallSym = Symbol::FromSignature("");
#endif
#endif