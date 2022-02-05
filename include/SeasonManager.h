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
		if (auto scripts = RE::ScriptEventSourceHolder::GetSingleton()) {
			scripts->AddEventSink<RE::TESActivateEvent>(GetSingleton());
			logger::info("Registered {}"sv, typeid(RE::TESActivateEvent).name());
		}
	}

	void LoadSettings();
	void LoadOrGenerateWinterFormSwap();
	void LoadFormSwaps();

	//Calendar is not initialized using savegame values when it is loaded from start
	void SaveSeason(std::string_view a_savePath);
	void LoadSeason(const std::string& a_savePath);
	void ClearSeason(std::string_view a_savePath) const;
	void CleanupSerializedSeasonList() const;

	bool UpdateSeason();

	[[nodiscard]] SEASON GetSeasonType();
	[[nodiscard]] bool CanSwapGrass();

	[[nodiscard]] std::pair<bool, std::string> CanSwapLOD(LOD_TYPE a_type);

	[[nodiscard]] bool IsLandscapeSwapAllowed();
	[[nodiscard]] bool IsSwapAllowed(RE::FormType a_formType);

	RE::TESBoundObject* GetSwapForm(const RE::TESForm* a_form);
	RE::TESLandTexture* GetSwapLandTexture(const RE::TESLandTexture* a_landTxst);
	RE::TESLandTexture* GetSwapLandTextureFromTextureSet(const RE::BGSTextureSet* a_txst);

	bool GetExterior();
	void SetExterior(bool a_isExterior);

protected:
	using EventResult = RE::BSEventNotifyControl;
	using SeasonPtr = std::optional<std::reference_wrapper<Season>>;

	SeasonPtr GetSeason();
	SeasonPtr GetCurrentSeason();

	static void LoadFormSwaps_Impl(Season& a_season);

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
			REL::Relocation<std::uintptr_t> load_interior{ REL::ID(13171), 0x2E6 };
			stl::write_thunk_call<SetInterior>(load_interior.address());

			REL::Relocation<std::uintptr_t> leave_interior{ REL::ID(13172), 0x2A };
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

	Season winter{ SEASON::kWinter, { "Winter", "WIN" } };
	Season spring{ SEASON::kSpring, { "Spring", "SPR" } };
	Season summer{ SEASON::kSummer, { "Summer", "SUM" } };
	Season autumn{ SEASON::kAutumn, { "Autumn", "AUT" } };

	SEASON currentSeason{ SEASON::kNone };
	SEASON lastSeason{ SEASON::kNone };

	std::atomic_bool isExterior{ false };

	bool loadedFromSave{ false };

	const wchar_t* serializedSeasonList{ L"Data/Seasons/Serialization.ini" };
};
