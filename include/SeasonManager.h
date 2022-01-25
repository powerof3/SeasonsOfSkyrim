#pragma once

#include "Seasons.h"

class SeasonManager
{
public:
	[[nodiscard]] static SeasonManager* GetSingleton()
	{
		static SeasonManager singleton;
		return std::addressof(singleton);
	}

	static void InstallHooks()
	{
		Hooks::Install();
	}

	void LoadSettings();

	void LoadOrGenerateWinterFormSwap();

	void LoadFormSwaps();

	[[nodiscard]] SEASON GetSeasonType();
    [[nodiscard]] bool CanSwapGrass();

	[[nodiscard]] std::pair<bool, std::string> CanSwapLOD();

	[[nodiscard]] bool IsLandscapeSwapAllowed();
	[[nodiscard]] bool IsSwapAllowed();
	[[nodiscard]] bool IsSwapAllowed(const RE::TESForm* a_form);

	RE::TESBoundObject* GetSwapForm(const RE::TESForm* a_form);
	RE::TESLandTexture* GetLandTexture(const RE::TESForm* a_form);
	RE::TESLandTexture* GetLandTextureFromTextureSet(const RE::TESForm* a_form);

protected:
	SeasonManager() = default;
	SeasonManager(const SeasonManager&) = delete;
	SeasonManager(SeasonManager&&) = delete;
	~SeasonManager() = default;

	SeasonManager& operator=(const SeasonManager&) = delete;
	SeasonManager& operator=(SeasonManager&&) = delete;

private:
	std::optional<Season> GetSeason();

	static void LoadFormSwaps_Impl(Season& a_season);

	struct Hooks
	{
		struct SetInterior
		{
			static void thunk(bool a_isInterior)
			{
				func(a_isInterior);

				GetSingleton()->isExterior = !a_isInterior;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		static void Install()
		{
			REL::Relocation<std::uintptr_t> load_interior{ REL::ID(13171), 0x2E6 };
			stl::write_thunk_call<SetInterior>(load_interior.address());

			REL::Relocation<std::uintptr_t> leave_interior{ REL::ID(13172), 0x2A };
			stl::write_thunk_call<SetInterior>(leave_interior.address());

			logger::info("Installed interior-exterior detection"sv);
		}
	};

	Season winter{ SEASON::kWinter, { "Winter", "WIN" } };
	Season spring{ SEASON::kSpring, { "Spring", "SPR" } };
	Season summer{ SEASON::kSummer, { "Summer", "SUM" } };
	Season autumn{ SEASON::kAutumn, { "Autumn", "AUT" } };

	SEASON_TYPE seasonType{ SEASON_TYPE::kSeasonal };

	std::atomic_bool isExterior{ false };
};
