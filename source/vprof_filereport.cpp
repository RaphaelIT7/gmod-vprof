#include <GarrysMod/InterfacePointers.hpp>
#include "main.h"
#include "detours.h"
#include <vprof.h>
#include <sstream>
#include <filesystem.h>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <tier0/dbg.h>

ConVar vprof_exportreport("vprof_exportreport", "1");

std::string GetCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H-%M-%S");

    return ss.str();
}

std::stringstream ss;
#ifdef ARCHITECTURE_X86
SpewOutputFunc_t last_spew;
#endif
void FinishDump()
{
#ifdef ARCHITECTURE_X86
	SpewOutputFunc(last_spew);
#endif

	IFileSystem* fs = InterfacePointers::FileSystem();
	if (!fs->IsDirectory("vprof", "MOD"))
	{
		if (fs->FileExists("vprof", "MOD"))
		{
			Msg("vprof/ is a file? Please delete it or disable vprof_exportreport.\n");
			return;
		}

		fs->CreateDirHierarchy("vprof", "MOD");
	}

	std::string filename = GetCurrentTime();
	filename = "vprof/" + filename + ".txt";
	FileHandle_t fh = fs->Open(filename.c_str(), "a+", "MOD");
	if (fh)
	{
		std::string str = ss.str();
		fs->Write(str.c_str(), str.length(), fh);  
		Msg("Wrote vprof report into %s\n", filename.c_str());

		fs->Close(fh);
	}

	ss.str("");
}

bool vprof_workaround = false;
#ifdef ARCHITECTURE_X86
static SpewRetval_t VProf_Spew(SpewType_t type, const char *msg)
{
	ss << msg;
	if (vprof_workaround && strcmp(msg, "******** END VPROF REPORT ********\n") == 0)
	{
		FinishDump();
	}
	return SPEW_CONTINUE;
}

void BeginDump()
{
	last_spew = GetSpewOutputFunc();
	SpewOutputFunc(VProf_Spew);
}

SpewOutputFunc_t original_spew = nullptr;
static SpewRetval_t VProfCheck_Spew(SpewType_t type, const char *msg)
{
	if (strcmp(msg, "******** BEGIN VPROF REPORT ********\n") == 0)
	{
		BeginDump();
		return VProf_Spew(type, msg);
	}

	return original_spew(type, msg);
}
#else
bool dumping = false;
Detouring::Hook detour_Msg;
void hook_Msg(PRINTF_FORMAT_STRING const tchar* pMsgFormat, ...)
{
	if (strcmp(pMsgFormat, "******** BEGIN VPROF REPORT ********\n") == 0)
	{
		dumping = true;
	}

	if (dumping)
	{
		va_list args;
		va_start(args, pMsgFormat);

		int size = vsnprintf(NULL, 0, pMsgFormat, args);
		if (size < 0) {
			va_end(args);
		} else {
			char* buffer = new char[size + 1];
			vsnprintf(buffer, size + 1, pMsgFormat, args);

			ss << buffer;

			delete[] buffer;
			va_end(args);
		}

		if (strcmp(pMsgFormat, "******** END VPROF REPORT ********\n") == 0)
		{
			dumping = false;
			FinishDump();
		}

		return;
	}

	va_list args;
	va_start(args, pMsgFormat);
	detour_Msg.GetTrampoline<TMsg>()(pMsgFormat, args);
	va_end(args);
}
#endif

#ifdef SYSTEM_WINDOWS
void AddWindowsWorkaround()
{
	Msg("[vprof] Applied a workaround for vprof_exportreport!\n"); // ToDo: Find out why Detouring::ClassProxy breaks for CVProfile
	vprof_workaround = true;
#ifdef ARCHITECTURE_X86
	original_spew = GetSpewOutputFunc();
	SpewOutputFunc(VProfCheck_Spew);
#else
	SourceSDK::ModuleLoader tier0_loader("tier0");
	CreateDetour(&detour_Msg, "Msg", tier0_loader.GetModule(), MsgSym, (void*)hook_Msg, DETOUR_VPROFEXPORT);
#endif
}

void RemoveWindowsWorkaround()
{
	vprof_workaround = false;
#ifdef ARCHITECTURE_X86
	if (original_spew)
		SpewOutputFunc(original_spew);
#else
	RemoveDetours(DETOUR_VPROFEXPORT);
#endif
}
#else
Detouring::Hook detour_CVProfile_OutputReport;
void hook_CVProfile_OutputReport(void* funky_class, int type, const tchar* pszStartMode, int budgetGroupID)
{
	if (!vprof_exportreport.GetBool())
	{
		detour_CVProfile_OutputReport.GetTrampoline<CVProfile_OutputReport>()(funky_class, type, pszStartMode, budgetGroupID);
		return;
	}

	BeginDump();
	detour_CVProfile_OutputReport.GetTrampoline<CVProfile_OutputReport>()(funky_class, type, pszStartMode, budgetGroupID);
	FinishDump();
}
#endif

void AddVProfExport()
{
	RemoveVProfExport();

#ifdef SYSTEM_WINDOWS
	AddWindowsWorkaround();
#else
	SourceSDK::ModuleLoader libtier0_loader("libtier0");
	CreateDetour(&detour_CVProfile_OutputReport, "CVProfile::OutputReport", libtier0_loader.GetModule(), CVProfile_OutputReportSym, (void*)hook_CVProfile_OutputReport, DETOUR_VPROFEXPORT);
#endif
}

void RemoveVProfExport()
{
#ifdef SYSTEM_WINDOWS
	RemoveWindowsWorkaround();
#else
	RemoveDetours(DETOUR_VPROFEXPORT);
#endif
}