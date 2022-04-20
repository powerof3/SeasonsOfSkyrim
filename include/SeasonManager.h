#pragma once

#include "Seasons.h"

class SeasonManager final : public RE::BSTEventSink<RE::TESActivateEvent>
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

	static void RegisterEvents()
	{
		if (const auto scripts = RE::ScriptEventSourceHolder::GetSingleton()) {
			scripts->AddEventSink<RE::TESActivateEvent>(GetSingleton());
			logger::info("Registered {}"sv, typeid(RE::TESActivateEvent).name());
		}
	}

	void LoadSettings();
	void LoadOrGenerateWinterFormSwap();
	void LoadSeasonData();

	//Calendar is not initialized using savegame values when it is loaded from start
	void SaveSeason(std::string_view a_savePath);
	void LoadSeason(const std::string& a_savePath);
	void ClearSeason(std::string_view a_savePath) const;
	void CleanupSerializedSeasonList() const;

	bool UpdateSeason();

	[[nodiscard]] SEASON GetCurrentSeasonType();
	[[nodiscard]] SEASON GetSeasonType();
	[[nodiscard]] bool CanApplySnowShader();

	[[nodiscard]] std::pair<bool, std::string> CanSwapLOD(LOD_TYPE a_type);

	[[nodiscard]] bool CanSwapLandscape();
	[[nodiscard]] bool CanSwapForm(RE::FormType a_formType);
	[[nodiscard]] bool CanSwapGrass(bool a_useAlt);

	RE::TESBoundObject* GetSwapForm(const RE::TESForm* a_form);
	template <class T>
	T* GetSwapForm(const RE::TESForm* a_form);

	RE::TESLandTexture* GetSwapLandTexture(const RE::TESLandTexture* a_landTxst);
	RE::TESLandTexture* GetSwapLandTexture(const RE::BGSTextureSet* a_txst);
	[[nodiscard]] bool GetUseAltGrass();

	[[nodiscard]] bool GetExterior();
	void SetExterior(bool a_isExterior);

protected:
	using MONTH = RE::Calendar::Month;
	using EventResult = RE::BSEventNotifyControl;

	Season* GetSeason();
	Season* GetCurrentSeason();

	void LoadMonthToSeasonMap(CSimpleIniA& a_ini);

	static void LoadSeasonData(Season& a_season, CSimpleIniA& a_settings);

	bool ShouldRegenerateWinterFormSwap() const;

	struct Hooks
	{
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
			REL::Relocation<std::uintptr_t> load_interior{ RELOCATION_ID(13171, 13316), OFFSET(0x2E6,0x46D) };
			stl::write_thunk_call<SetInterior>(load_interior.address());

			REL::Relocation<std::uintptr_t> leave_interior{ RELOCATION_ID(13172, 13317), OFFSET(0x2A,0x1E) };
			stl::write_thunk_call<SetInterior>(leave_interior.address());

			logger::info("Installed interior-exterior detection"sv);
		}
	};

	EventResult ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>*) override;

private:
	SeasonManager() = default;
	SeasonManager(const SeasonManager&) = delete;
	SeasonManager(SeasonManager&&) = delete;
	~SeasonManager() override = default;

	SeasonManager& operator=(const SeasonManager&) = delete;
	SeasonManager& operator=(SeasonManager&&) = delete;

	SEASON_TYPE seasonType{ SEASON_TYPE::kSeasonal };

	std::map<MONTH, SEASON> monthToSeasons{
		{ MONTH::kMorningStar, SEASON::kWinter },
		{ MONTH::kSunsDawn, SEASON::kWinter },
		{ MONTH::kFirstSeed, SEASON::kSpring },
		{ MONTH::kRainsHand, SEASON::kSpring },
		{ MONTH::kSecondSeed, SEASON::kSpring },
		{ MONTH::kMidyear, SEASON::kSummer },
		{ MONTH::kSunsHeight, SEASON::kSummer },
		{ MONTH::kLastSeed, SEASON::kSummer },
		{ MONTH::kHearthfire, SEASON::kAutumn },
		{ MONTH::kFrostfall, SEASON::kAutumn },
		{ MONTH::kSunsDusk, SEASON::kAutumn },
		{ MONTH::kEveningStar, SEASON::kWinter }
	};

	frozen::map<MONTH, std::pair<std::string_view, std::string_view>, 12> monthNames{
		{ MONTH::kMorningStar, std::make_pair("Morning Star"sv, ";January"sv) },
		{ MONTH::kSunsDawn, std::make_pair("Sun's Dawn"sv, ";February"sv) },
		{ MONTH::kFirstSeed, std::make_pair("First Seed"sv, ";March"sv) },
		{ MONTH::kRainsHand, std::make_pair("Rain's Hand"sv, ";April"sv) },
		{ MONTH::kSecondSeed, std::make_pair("Second Seed"sv, ";May"sv) },
		{ MONTH::kMidyear, std::make_pair("Mid Year"sv, ";June"sv) },
		{ MONTH::kSunsHeight, std::make_pair("Sun's Height"sv, ";July"sv) },
		{ MONTH::kLastSeed, std::make_pair("Last Seed"sv, ";August"sv) },
		{ MONTH::kHearthfire, std::make_pair("Hearthfire"sv, ";September"sv) },
		{ MONTH::kFrostfall, std::make_pair("Frost Fall"sv, ";October"sv) },
		{ MONTH::kSunsDusk, std::make_pair("Sun's Dusk"sv, ";November"sv) },
		{ MONTH::kEveningStar, std::make_pair("Evening Star"sv, ";December"sv) }
	};

	Season winter{ SEASON::kWinter, { "Winter", "WIN" } };
	Season spring{ SEASON::kSpring, { "Spring", "SPR" } };
	Season summer{ SEASON::kSummer, { "Summer", "SUM" } };
	Season autumn{ SEASON::kAutumn, { "Autumn", "AUT" } };

	SEASON currentSeason{ SEASON::kNone };
	SEASON lastSeason{ SEASON::kNone };

	std::atomic_bool isExterior{ false };

	bool loadedFromSave{ false };

	const wchar_t* settings{ L"Data/SKSE/Plugins/po3_SeasonsOfSkyrim.ini" };
	const wchar_t* serializedSeasonList{ L"Data/Seasons/Serialization.ini" };
};

template <class T>
T* SeasonManager::GetSwapForm(const RE::TESForm* a_form)
{
	auto swapForm = GetSwapForm(a_form);
	return swapForm ? swapForm->As<T>() : nullptr;
}
