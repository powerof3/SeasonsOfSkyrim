#include "Papyrus.h"
#include "SeasonManager.h"

namespace Papyrus
{
	bool Bind(VM* a_vm)
	{
		if (!a_vm) {
			logger::critical("couldn't get VM State"sv);
			return false;
		}

		logger::info("{:*^30}", "FUNCTIONS"sv);

		Seasons::Bind(*a_vm);

		return true;
	}

	std::uint32_t Seasons::GetCurrentSeason(VM*, StackID, RE::StaticFunctionTag*)
	{
		return stl::to_underlying(SeasonManager::GetSingleton()->GetCurrentSeasonType());
	}

	void Seasons::Bind(VM& a_vm)
	{
		constexpr auto script = "SeasonsOfSkyrim"sv;

		a_vm.RegisterFunction("GetCurrentSeason", "SeasonsOfSkyrim", GetCurrentSeason, true);

		logger::info("Registered season functions"sv);
	}
}
