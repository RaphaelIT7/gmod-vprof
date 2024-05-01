#include "detours.h"
#include <map>
#include <vector>

SymbolFinder symfinder;
void* GetFunction(void* module, Symbol symbol)
{
	return symfinder.Resolve(module, symbol.name.c_str(), symbol.length);
}

std::map<DETOUR_CATEGOY, std::vector<Detouring::Hook*>> detours = {};
void CreateDetour(Detouring::Hook* hook, const char* name, void* module, Symbol symbol, void* hook_func, DETOUR_CATEGOY category)
{
	void* func = symfinder.Resolve(module, symbol.name.c_str(), symbol.length);
	if (!CheckFunction(func, name))
		return;

	hook->Create(func, hook_func);
	hook->Enable();

	detours[category].push_back(hook);

	if (!hook->IsValid()) {
		Msg("Failed to detour %s!\n", name);
	}
}

void RemoveDetours(DETOUR_CATEGOY category)
{
	if (category == DETOUR_ALL)
	{
		for (auto& [_, cat_detours] : detours) {
			for (Detouring::Hook* hook : cat_detours) {
				if (hook->IsEnabled())
				{
					hook->Disable();
					hook->Destroy();
				}
			}
		}
		detours.clear();
	} else {
		for (Detouring::Hook* hook : detours[category]) {
			if (hook->IsEnabled())
			{
				hook->Disable();
				hook->Destroy();
			}
		}
		detours[category].clear();
	}
}