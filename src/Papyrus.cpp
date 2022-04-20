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

    bool Seasons::GetSeasonOverride(VM*, StackID, RE::StaticFunctionTag*, std::uint32_t a_season)
	{
		return stl::to_underlying(SeasonManager::GetSingleton()->GetSeasonOverride());
	}

    void Seasons::SetSeasonOverride(VM*, StackID, RE::StaticFunctionTag*, std::uint32_t a_season)
	{
		SeasonManager::GetSingleton()->SetSeasonOverride(static_cast<SEASON>(a_season));
	}

    void Seasons::ClearSeasonOverride(VM*, StackID, RE::StaticFunctionTag*, std::uint32_t a_season)
	{
		SeasonManager::GetSingleton()->SetSeasonOverride(SEASON::kNone);
	}

    void Seasons::Bind(VM& a_vm)
	{
		constexpr auto script = "SeasonsOfSkyrim"sv;

		a_vm.RegisterFunction("GetCurrentSeason", script, GetCurrentSeason, true);

		a_vm.RegisterFunction("GetSeasonOverride", script, GetSeasonOverride);
		a_vm.RegisterFunction("SetSeasonOverride", script, SetSeasonOverride);
		a_vm.RegisterFunction("ClearSeasonOverride", script, ClearSeasonOverride);

		logger::info("Registered season functions"sv);
	}
}
