#include "SeasonManager.h"
#include "Papyrus.h"

Season* SeasonManager::GetSeasonImpl(SEASON_TYPE a_season)
{
	switch (a_season) {
	case SEASON_TYPE::kWinter:
		return &winter;
	case SEASON_TYPE::kSpring:
		return &spring;
	case SEASON_TYPE::kSummer:
		return &summer;
	case SEASON_TYPE::kAutumn:
		return &autumn;
	default:
		return nullptr;
	}
}

Season* SeasonManager::GetCurrentSeason(bool a_ignoreOverride)
{
	if (!a_ignoreOverride && seasonOverride != SEASON_TYPE::kNone) {
		return GetSeasonImpl(seasonOverride);
	}

	switch (GetSeasonMode()) {
	case SEASON_MODE::kPermanentWinter:
		return &winter;
	case SEASON_MODE::kPermanentSpring:
		return &spring;
	case SEASON_MODE::kPermanentSummer:
		return &summer;
	case SEASON_MODE::kPermanentAutumn:
		return &autumn;
	case SEASON_MODE::kSeasonal:
		{
			const auto calendar = RE::Calendar::GetSingleton();
			const auto month = calendar ? calendar->GetMonth() : 7;

			if (const auto it = monthToSeasons.find(static_cast<MONTH>(month)); it != monthToSeasons.end()) {
				return GetSeasonImpl(it->second);
			}

			return nullptr;
		}
	default:
		return nullptr;
	}
}

bool SeasonManager::UpdateSeason()
{
	bool shouldUpdate = false;

	if (loadedFromSave) {
		shouldUpdate = true;
	}

	if (seasonOverride != SEASON_TYPE::kNone) {
		const auto tempLastSeason = lastSeason;
		lastSeason = seasonOverride;

		if (!shouldUpdate) {
			shouldUpdate = seasonOverride != tempLastSeason;
		}
		if (!loadedFromSave && shouldUpdate) {
			Papyrus::Events::Manager::GetSingleton()->seasonChange.QueueEvent(std::to_underlying(tempLastSeason), std::to_underlying(seasonOverride), true);
		}

	} else {
		lastSeason = currentSeason;

		if (!shouldUpdate) {
			const auto season = GetCurrentSeason();
			currentSeason = season ? season->GetType() : SEASON_TYPE::kNone;

			shouldUpdate = currentSeason != lastSeason;
		}
		if (!loadedFromSave && shouldUpdate) {
			Papyrus::Events::Manager::GetSingleton()->seasonChange.QueueEvent(std::to_underlying(lastSeason), std::to_underlying(currentSeason), false);
		}
	}

	if (loadedFromSave) {
		loadedFromSave = false;
	}

	return shouldUpdate;
}

Season* SeasonManager::GetSeason()
{
	if (!GetExterior()) {
		return nullptr;
	}

	if (seasonOverride != SEASON_TYPE::kNone) {
		return GetSeasonImpl(seasonOverride);
	} else {
		if (currentSeason == SEASON_TYPE::kNone) {
			UpdateSeason();
		}
		return GetSeasonImpl(currentSeason);
	}
}

void SeasonManager::LoadSettings()
{
	REX::INFO("{:*^30}", "SETTINGS");

	ResetOutdatedSettings();

	const auto store = REX::FIniSettingStore::GetSingleton();
	store->Init(settings, "");
	store->Load();
	store->Save();

	RebuildMonthToSeasonMap();

	using record_type = FormSwapMap::RECORD;

	mainWINSwap.skip = skipWINSwap.GetValue();
	mainWINSwap.skipRecords[std::to_underlying(record_type::kLandTextures)] = skipLandTextures.GetValue();
	mainWINSwap.skipRecords[std::to_underlying(record_type::kActivators)] = skipActivators.GetValue();
	mainWINSwap.skipRecords[std::to_underlying(record_type::kFurniture)] = skipFurniture.GetValue();
	mainWINSwap.skipRecords[std::to_underlying(record_type::kMovableStatics)] = skipMovableStatics.GetValue();
	mainWINSwap.skipRecords[std::to_underlying(record_type::kStatics)] = skipStatics.GetValue();
	mainWINSwap.skipRecords[std::to_underlying(record_type::kTrees)] = skipTrees.GetValue();

	REX::INFO("Season mode is {}", seasonType.GetValue());
}

void SeasonManager::RebuildMonthToSeasonMap()
{
	monthToSeasons.clear();

	for (const auto month : enum_range(MONTH::kMorningStar, MONTH::kTotal)) {
		const auto value = monthSettings[std::to_underlying(month)].GetValue();
		if (value >= std::to_underlying(SEASON_TYPE::kTotal)) {
			REX::WARN("Invalid season {} for month {}, defaulting to none", value, std::to_underlying(month));
			monthToSeasons.emplace(month, SEASON_TYPE::kNone);
		} else {
			monthToSeasons.emplace(month, static_cast<SEASON_TYPE>(value));
		}
	}
}

void SeasonManager::ResetOutdatedSettings()
{
	CSimpleIniA ini;
	ini.SetUnicode();

	if (ini.LoadFile(settings) < SI_OK) {
		return;
	}

	// delete and recreate settings if new settings are not found
	if (ini.GetValue("Settings", "Morning Star") == nullptr || ini.GetValue("Winter", "Flora") == nullptr) {
		ini.Delete("Settings", nullptr);
		ini.Delete("Winter", nullptr);
		ini.Delete("Spring", nullptr);
		ini.Delete("Summer", nullptr);
		ini.Delete("Autumn", nullptr);
		(void)ini.SaveFile(settings);
	}
}

bool SeasonManager::ShouldRegenerateWinterFormSwap() const
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(serializedSeasonList);

	//1.6.0 - delete old serialized value to force regeneration
	ini.DeleteValue("Game", "Mod Count", nullptr);
	//2.0.0 - delete old serialized value to force regeneration
	ini.DeleteValue("Game", "Total Mod Count", nullptr);

	const auto actualHash = util::get_load_order_hash();
	const auto expectedHash = REX::STR::TO_NUM<size_t>(ini.GetValue("Game", "uLoadOrderHash", "0"));

	const auto shouldRegenerate = actualHash != expectedHash;

	if (shouldRegenerate) {
		ini.SetValue("Game", "uLoadOrderHash", std::to_string(actualHash).c_str(), nullptr);
		if (expectedHash != 0) {
			REX::INFO("\tLoad order has changed since last session, regenerating main WIN formswap");
		} else {
			REX::INFO("\tRegenerating main WIN formswap since last update");
		}
		(void)ini.SaveFile(serializedSeasonList);
	} else {
		REX::INFO("\tLoad order has not changed since last session");
	}

	return shouldRegenerate;
}

void SeasonManager::LoadOrGenerateWinterFormSwap()
{
	if (mainWINSwap.skip) {
		REX::INFO("Main WIN formswap loading disabled in config");
		return;
	}

	constexpr auto path = L"Data/Seasons/MainFormSwap_WIN.ini";

	REX::INFO("Loading main WIN formswap settings");

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.SetMultiKey();
	ini.SetAllowKeyOnly();

	ini.LoadFile(path);

	auto& winFormSwapMap = winter.GetFormSwapMap();

	if (winFormSwapMap.GenerateFormSwaps(ini, ShouldRegenerateWinterFormSwap())) {
		(void)ini.SaveFile(path);
	} else {
		for (const auto record : FormSwapMap::standard_records()) {
			auto type = FormSwapMap::get_name(record);

			if (mainWINSwap.skipRecords[std::to_underlying(record)]) {
				REX::INFO("\t\t[{}] skipping...", type);
				continue;
			}

			CSimpleIniA::TNamesDepend values;
			ini.GetAllKeys(type.data(), values);
			values.sort(CSimpleIniA::Entry::LoadOrder());

			if (!values.empty()) {
				REX::INFO("\t\t[{}] read {} variants", type, values.size());

				std::vector<std::string> vec;
				std::ranges::transform(values, std::back_inserter(vec), [&](const auto& val) { return val.pItem; });

				winFormSwapMap.LoadFormSwaps(record, vec);
			}
		}
	}
}

void SeasonManager::LoadSeasonData(Season& a_season, CSimpleIniA& a_settings)
{
	std::vector<std::string> configs;

	const auto& [type, suffix] = a_season.GetID();

	REX::INFO("{}", type);

	for (constexpr auto folder = R"(Data\Seasons)"; const auto& entry : std::filesystem::directory_iterator(folder)) {
		if (entry.is_regular_file() && entry.path().extension() == ".ini"sv) {
			const auto& path = entry.path().string();
			const auto& pathStem = entry.path().stem().string();

			if (pathStem.ends_with(suffix) && !path.contains("MainFormSwap"sv)) {
				configs.push_back(path);
			}
		}
	}

	if (configs.empty()) {
		REX::WARN("\tNo .ini files with _{} suffix were found in Data/Seasons folder, skipping {} formswaps", suffix, suffix == "WIN"sv ? "secondary" : "all");
		return;
	}

	REX::INFO("\t{} matching inis found", configs.size());

	std::ranges::sort(configs);

	for (auto& path : configs) {
		REX::INFO("\tINI : {}", path);

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetMultiKey();
		ini.SetAllowKeyOnly();

		if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
			REX::ERROR("\t\tcouldn't read INI");
			continue;
		}

		a_season.LoadData(ini);
	}

	//save worldspaces to settings so DynDOLOD can read them
	a_season.SaveData(a_settings);
}

void SeasonManager::LoadSeasonData()
{
	CSimpleIniA settingsINI;
	settingsINI.SetUnicode();

	settingsINI.LoadFile(settings);

	ForEachSeason([&](SEASON_TYPE, Season& a_season) {
		LoadSeasonData(a_season, settingsINI);
	});

	(void)settingsINI.SaveFile(settings);
}

void SeasonManager::LoadValidWorldspaces()
{
	ForEachSeason([](SEASON_TYPE, Season& a_season) {
		a_season.LoadWorldspaces();
	});
}

void SeasonManager::CheckLODExists()
{
	REX::INFO("{:*^30}", "LOD");

	ForEachSeason([](SEASON_TYPE, Season& a_season) {
		a_season.CheckLODExists();
	});
}

void SeasonManager::SaveSeason(std::string_view a_savePath)
{
	if (const auto player = RE::PlayerCharacter::GetSingleton(); !player->parentCell || !player->parentCell->IsExteriorCell()) {
		return;
	}

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(serializedSeasonList);

	const auto season = GetCurrentSeason(true);
	currentSeason = season ? season->GetType() : SEASON_TYPE::kNone;

	const auto seasonData = std::format("{}|{}", std::to_underlying(currentSeason), std::to_underlying(seasonOverride));
	ini.SetValue("Saves", a_savePath.data(), seasonData.c_str(), nullptr);

	(void)ini.SaveFile(serializedSeasonList);
}

void SeasonManager::LoadSeason(const std::string& a_savePath)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(serializedSeasonList);

	const auto seasonData = REX::STR::SPLIT(ini.GetValue("Saves", a_savePath.c_str(), "3"), "|");
	if (seasonData.size() == 2) {
		currentSeason = REX::STR::TO_NUM<SEASON_TYPE>(seasonData[0]);
		seasonOverride = REX::STR::TO_NUM<SEASON_TYPE>(seasonData[1]);
	} else {
		currentSeason = REX::STR::TO_NUM<SEASON_TYPE>(seasonData[0]);
		seasonOverride = SEASON_TYPE::kNone;
	}

	loadedFromSave = true;

	(void)ini.SaveFile(serializedSeasonList);
}

void SeasonManager::ClearSeason(std::string_view a_savePath) const
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(serializedSeasonList);

	ini.DeleteValue("Saves", a_savePath.data(), nullptr);

	(void)ini.SaveFile(serializedSeasonList);
}

void SeasonManager::CleanupSerializedSeasonList() const
{
	constexpr auto get_save_directory = []() -> std::optional<std::filesystem::path> {
		if (auto path = SKSE::log::log_directory()) {
			path->remove_filename();  // remove "/SKSE"
			path->append(*"sLocalSavePath:General"_ini);
			return path;
		}
		return std::nullopt;
	};

	const auto directory = get_save_directory();
	if (!directory) {
		return;
	}

	REX::INFO("{:*^30}", "SAVES");

	REX::INFO("Save directory is {}", directory->string());

	CSimpleIniA ini;
	ini.SetUnicode();

	if (const auto rc = ini.LoadFile(serializedSeasonList); rc < 0) {
		return;
	}

	CSimpleIniA::TNamesDepend values;
	ini.GetAllKeys("Saves", values);
	values.sort(CSimpleIniA::Entry::LoadOrder());

	if (!values.empty()) {
		std::vector<std::string> badSaves;
		badSaves.reserve(values.size());
		for (const auto& key : values) {
			if (auto save = std::format("{}{}.ess", directory->string(), key.pItem); !std::filesystem::exists(save)) {
				badSaves.emplace_back(key.pItem);
			}
		}
		for (auto& badSave : badSaves) {
			ini.DeleteValue("Saves", badSave.c_str(), nullptr);
		}
	}

	(void)ini.SaveFile(serializedSeasonList);
}

SEASON_MODE SeasonManager::GetSeasonMode() const
{
	return static_cast<SEASON_MODE>(seasonType.GetValue());
}

SEASON_TYPE SeasonManager::GetCurrentSeasonType()
{
	const auto season = GetCurrentSeason();
	return season ? season->GetType() : SEASON_TYPE::kNone;
}

SEASON_TYPE SeasonManager::GetSeasonType()
{
	const auto season = GetSeason();
	return season ? season->GetType() : SEASON_TYPE::kNone;
}

bool SeasonManager::CanApplySnowShader()
{
	const auto season = GetSeason();
	return season ? season->CanApplySnowShader() : false;
}

std::pair<bool, std::string_view> SeasonManager::CanSwapLOD(LOD_TYPE a_type)
{
	const auto season = GetSeason();
	return season ? std::make_pair(season->CanSwapLOD(a_type), season->GetID().suffix) : std::make_pair(false, ""sv);
}

RE::TESBoundObject* SeasonManager::GetSwapForm(const RE::TESForm* a_form)
{
	const auto season = GetSeason();
	return season && season->CanSwapForm(a_form->GetFormType()) ? season->GetFormSwapMap().GetSwapForm(a_form) : nullptr;
}

RE::TESLandTexture* SeasonManager::GetSwapLandTexture(const RE::TESLandTexture* a_landTxst)
{
	const auto season = GetSeason();
	return season && season->CanSwapLandscape() ? season->GetFormSwapMap().GetSwapLandTexture(a_landTxst) : nullptr;
}

RE::TESLandTexture* SeasonManager::GetSwapLandTexture(const RE::BGSTextureSet* a_txst)
{
	const auto season = GetSeason();
	return season && season->CanSwapLandscape() ? season->GetFormSwapMap().GetSwapLandTexture(a_txst) : nullptr;
}

RE::TESLandTexture* SeasonManager::GetSwapLandTextureForGrass(const RE::TESLandTexture* a_landTxst)
{
	const auto season = GetSeason();
	return season && season->CanSwapForm(RE::FormType::Grass) ? season->GetFormSwapMap().GetSwapLandTexture(a_landTxst) : nullptr;
}

bool SeasonManager::GetExterior()
{
	return isExterior;
}

void SeasonManager::SetExterior(bool a_isExterior)
{
	isExterior = a_isExterior;
}

SEASON_TYPE SeasonManager::GetSeasonOverride() const
{
	return seasonOverride;
}

bool SeasonManager::PreferMultipass() const
{
	return preferMultipass;
}

void SeasonManager::SetSeasonOverride(SEASON_TYPE a_season)
{
	seasonOverride = a_season;
}

SeasonManager::EventResult SeasonManager::ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>*)
{
	if (!a_event || GetExterior()) {
		return EventResult::kContinue;
	}

	constexpr auto is_teleport_door = [](auto&& a_ref, auto&& a_object) {
		return a_ref && a_ref->IsPlayerRef() && a_object && a_object->extraList.HasType(RE::ExtraDataType::kTeleport);
	};

	if (!is_teleport_door(a_event->actionRef, a_event->objectActivated)) {
		return EventResult::kContinue;
	}

	if (const auto tes = RE::TES::GetSingleton(); UpdateSeason()) {
		tes->PurgeBufferedCells();
	}

	return EventResult::kContinue;
}
