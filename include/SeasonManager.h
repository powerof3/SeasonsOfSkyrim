#pragma once

#include "Seasons.h"

class SeasonManager final :
	public REX::TSingleton<SeasonManager>,
	public RE::BSTEventSink<RE::TESActivateEvent>
{
public:
	static void InstallHooks()
	{
		Hooks::Install();
	}

	void RegisterEvents()
	{
		REX::INFO("{:*^30}", "EVENTS");

		if (const auto scripts = RE::ScriptEventSourceHolder::GetSingleton()) {
			scripts->AddEventSink<RE::TESActivateEvent>(this);
			REX::INFO("Registered {}"sv, typeid(RE::TESActivateEvent).name());
		}
	}

	void LoadSettings();
	void LoadOrGenerateWinterFormSwap();
	void LoadSeasonData();
	void LoadValidWorldspaces();
	void CheckLODExists();

	//Calendar is not initialized using savegame values when it is loaded from start
	void SaveSeason(std::string_view a_savePath);
	void LoadSeason(const std::string& a_savePath);
	void ClearSeason(std::string_view a_savePath) const;
	void CleanupSerializedSeasonList() const;

	bool UpdateSeason();

	[[nodiscard]] SEASON_MODE GetSeasonMode() const;
	[[nodiscard]] SEASON_TYPE GetCurrentSeasonType();
	[[nodiscard]] SEASON_TYPE GetSeasonType();
	[[nodiscard]] bool        CanApplySnowShader();

	[[nodiscard]] std::pair<bool, std::string_view> CanSwapLOD(LOD_TYPE a_type);

	RE::TESBoundObject* GetSwapForm(const RE::TESForm* a_form);
	template <class T>
	T* GetSwapForm(const RE::TESForm* a_form);

	RE::TESLandTexture* GetSwapLandTexture(const RE::TESLandTexture* a_landTxst);
	RE::TESLandTexture* GetSwapLandTexture(const RE::BGSTextureSet* a_txst);
	RE::TESLandTexture* GetSwapLandTextureForGrass(const RE::TESLandTexture* a_landTxst);

	[[nodiscard]] bool GetExterior();
	void               SetExterior(bool a_isExterior);

	SEASON_TYPE GetSeasonOverride() const;
	void        SetSeasonOverride(SEASON_TYPE a_season);

	bool PreferMultipass() const;

protected:
	using MONTH = RE::Calendar::Month;
	using EventResult = RE::BSEventNotifyControl;

	void ForEachSeason(auto&& func)
	{
		for (const auto type : enum_range(SEASON_TYPE::kWinter, SEASON_TYPE::kTotal)) {
			func(type, *GetSeasonImpl(type));
		}
	}

	Season* GetSeason();
	Season* GetCurrentSeason(bool a_ignoreOverride = false);
	Season* GetSeasonImpl(SEASON_TYPE a_season);

	void RebuildMonthToSeasonMap();
	void ResetOutdatedSettings();

	static void LoadSeasonData(Season& a_season, CSimpleIniA& a_settings);

	bool ShouldRegenerateWinterFormSwap() const;

	struct Hooks
	{
		template <std::size_t N>
		struct SetInterior
		{
			static void thunk(bool a_isInterior)
			{
				func(a_isInterior);

				const auto manager = GetSingleton();
				manager->SetExterior(!a_isInterior);

				if (!a_isInterior) {
					manager->UpdateSeason();
				}
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		static void Install()
		{
			REL::Relocation<std::uintptr_t> load_interior{ RELOCATION_ID(13171, 13316), OFFSET(0x2E6, 0x46D) };
			stl::write_thunk_call<SetInterior<0>>(load_interior.address());

			REL::Relocation<std::uintptr_t> leave_interior{ RELOCATION_ID(13172, 13317), OFFSET(0x2A, 0x1E) };
			stl::write_thunk_call<SetInterior<1>>(leave_interior.address());

			REX::INFO("Installed interior-exterior detection"sv);
		}
	};

	EventResult ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>*) override;

private:
	static constexpr auto settings{ R"(Data\SKSE\Plugins\po3_SeasonsOfSkyrim.ini)" };
	static constexpr auto serializedSeasonList{ R"(Data\Seasons\Serialization.ini)" };

	std::map<MONTH, SEASON_TYPE> monthToSeasons{};

	Setting::U32 seasonType{
		"Settings", "Season Type",
		";0 - disabled\n;1 - permanent winter\n;2 - permanent spring\n"
		";3 - permanent summer\n;4 - permanent autumn\n;5 - seasonal"sv,
		std::to_underlying(SEASON_MODE::kSeasonal)
	};

	std::array<Setting::U32, MONTH::kTotal> monthSettings{
		Setting::U32{ "Settings", "Morning Star",
			";0 - none\n;1 - winter\n;2 - spring\n;3 - summer\n;4 - autumn\n\n;January"sv,
			std::to_underlying(SEASON_TYPE::kWinter) },
		{ "Settings", "Sun's Dawn", ";February"sv, std::to_underlying(SEASON_TYPE::kWinter) },
		Setting::U32{ "Settings", "First Seed", ";March"sv, std::to_underlying(SEASON_TYPE::kSpring) },
		Setting::U32{ "Settings", "Rain's Hand", ";April"sv, std::to_underlying(SEASON_TYPE::kSpring) },
		Setting::U32{ "Settings", "Second Seed", ";May"sv, std::to_underlying(SEASON_TYPE::kSpring) },
		Setting::U32{ "Settings", "Mid Year", ";June"sv, std::to_underlying(SEASON_TYPE::kSummer) },
		Setting::U32{ "Settings", "Sun's Height", ";July"sv, std::to_underlying(SEASON_TYPE::kSummer) },
		Setting::U32{ "Settings", "Last Seed", ";August"sv, std::to_underlying(SEASON_TYPE::kSummer) },
		Setting::U32{ "Settings", "Hearthfire", ";September"sv, std::to_underlying(SEASON_TYPE::kAutumn) },
		Setting::U32{ "Settings", "Frost Fall", ";October"sv, std::to_underlying(SEASON_TYPE::kAutumn) },
		Setting::U32{ "Settings", "Sun's Dusk", ";November"sv, std::to_underlying(SEASON_TYPE::kAutumn) },
		Setting::U32{ "Settings", "Evening Star", ";December"sv, std::to_underlying(SEASON_TYPE::kWinter) },
	};

	Setting::Bool preferMultipass{ "Settings", "Prefer Multipass", ";Use multipass snow shader when applying snow shader to objects if possible", true };

	Setting::Bool skipWINSwap{ "Winter", "Ignore auto generated WIN formswap", ";Autogenerated winter formswap config will not be applied."sv, false };
	Setting::Bool skipLandTextures{ "Winter", "Skip Land Textures", ";Skip loading these form types from autogenerated winter formswap."sv, false };
	Setting::Bool skipActivators{ "Winter", "Skip Activator", "", false };
	Setting::Bool skipFurniture{ "Winter", "Skip Furniture", "", false };
	Setting::Bool skipMovableStatics{ "Winter", "Skip Movable Statics", "", false };
	Setting::Bool skipStatics{ "Winter", "Skip Statics", "", false };
	Setting::Bool skipTrees{ "Winter", "Skip Tree", "", false };

	Season winter{ SEASON_TYPE::kWinter, { "Winter", "WIN" }, true };
	Season spring{ SEASON_TYPE::kSpring, { "Spring", "SPR" }, false };
	Season summer{ SEASON_TYPE::kSummer, { "Summer", "SUM" }, false };
	Season autumn{ SEASON_TYPE::kAutumn, { "Autumn", "AUT" }, false };

	SEASON_TYPE currentSeason{ SEASON_TYPE::kNone };
	SEASON_TYPE lastSeason{ SEASON_TYPE::kNone };
	SEASON_TYPE seasonOverride{ SEASON_TYPE::kNone };

	std::atomic_bool isExterior{ false };
	bool             loadedFromSave{ false };

	struct
	{
		bool                                                              skip{ false };
		std::array<bool, std::to_underlying(FormSwapMap::RECORD::kFlora)> skipRecords{};
	} mainWINSwap;
};

template <class T>
T* SeasonManager::GetSwapForm(const RE::TESForm* a_form)
{
	auto swapForm = GetSwapForm(a_form);
	return swapForm ? swapForm->As<T>() : nullptr;
}
