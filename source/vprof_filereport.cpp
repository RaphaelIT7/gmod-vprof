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

std::stringstream ss;
static SpewRetval_t VProf_Spew(SpewType_t type, const char *msg)
{
	ss << msg;
	return SPEW_CONTINUE;
}

Detouring::Hook detour_CVProfile_OutputReport;
void hook_CVProfile_OutputReport(void* funky_class, int type, const tchar* pszStartMode, int budgetGroupID)
{
	if (!vprof_exportreport.GetBool())
	{
		detour_CVProfile_OutputReport.GetTrampoline<CVProfile_OutputReport>()(funky_class, type, pszStartMode, budgetGroupID);
		return;
	}


	SpewOutputFunc_t original_spew = GetSpewOutputFunc();

	original_spew = GetSpewOutputFunc();
	SpewOutputFunc(VProf_Spew);

	detour_CVProfile_OutputReport.GetTrampoline<CVProfile_OutputReport>()(funky_class, type, pszStartMode, budgetGroupID);

	SpewOutputFunc(original_spew);

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

void AddVProfExport()
{
	RemoveVProfExport();

	SourceSDK::ModuleLoader libtier0_loader("libtier0");
	CreateDetour(&detour_CVProfile_OutputReport, "CVProfile::OutputReport", libtier0_loader.GetModule(), CVProfile_OutputReportSym, (void*)hook_CVProfile_OutputReport, DETOUR_VPROFEXPORT);
}

void RemoveVProfExport()
{
	RemoveDetours(DETOUR_VPROFEXPORT);
}