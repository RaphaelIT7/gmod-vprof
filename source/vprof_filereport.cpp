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

ConVar vprof_exportreport("vprof_exportreport", "1");

std::string GetCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H-%M-%S");

    return ss.str();
}

SpewOutputFunc_t last_spew;
void BeginDump()
{
	last_spew = GetSpewOutputFunc();
	SpewOutputFunc(VProf_Spew);
}

void FinishDump()
{
	SpewOutputFunc(last_spew);

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

std::stringstream ss;
bool vprof_workaround = false;
SpewOutputFunc_t original_spew;
static SpewRetval_t VProf_Spew(SpewType_t type, const char *msg)
{
	ss << msg;
	if (vprof_workaround && strcmp(msg, "******** END VPROF REPORT ********\n") == 0)
	{
		FinishDump();
	}
	return SPEW_CONTINUE;
}

static SpewRetval_t VProfCheck_Spew(SpewType_t type, const char *msg)
{
	if (strcmp(msg, "******** BEGIN VPROF REPORT ********\n") == 0)
	{
		BeginDump();
		return VProf_Spew(type, msg);
	}

	return original_spew(type, msg);
}

#ifdef SYSTEM_WINDOWS
void AddWindowsWorkaround()
{
	Msg("[vprof] Applied a workaround for vprof_exportreport!\n"); // ToDo: Find out why Detouring::ClassProxy breaks for CVProfile
	vprof_workaround = true;
	original_spew = GetSpewOutputFunc();
	SpewOutputFunc(VProfCheck_Spew);
}

void RemoveWindowsWorkaround()
{
	vprof_workaround = false;
	SpewOutputFunc(original_spew);
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