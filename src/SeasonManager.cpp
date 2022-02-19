#include "SeasonManager.h"

SeasonManager::SeasonPtr SeasonManager::GetCurrentSeason()
{
	switch (seasonType) {
	case SEASON_TYPE::kPermanentWinter:
		return winter;
	case SEASON_TYPE::kPermanentSpring:
		return spring;
	case SEASON_TYPE::kPermanentSummer:
		return summer;
	case SEASON_TYPE::kPermanentAutumn:
		return autumn;
	case SEASON_TYPE::kSeasonal:
		{
			const auto calendar = RE::Calendar::GetSingleton();
			const auto month = calendar ? calendar->GetMonth() : 7;

			if (const auto it = monthToSeasons.find(static_cast<MONTH>(month)); it != monthToSeasons.end()) {
				switch (it->second) {
				case SEASON::kWinter:
					return winter;
				case SEASON::kSpring:
					return spring;
				case SEASON::kSummer:
					return summer;
				case SEASON::kAutumn:
					return autumn;
				default:
					return std::nullopt;
				}
			}

			return std::nullopt;
		}
	default:
		return std::nullopt;
	}
}

bool SeasonManager::UpdateSeason()
{
	lastSeason = currentSeason;

	if (loadedFromSave) {
		loadedFromSave = false;
		return true;
	}

	const auto season = GetCurrentSeason();
	currentSeason = season ? season->get().GetType() : SEASON::kNone;

	return currentSeason != lastSeason;
}

SeasonManager::SeasonPtr SeasonManager::GetSeason()
{
	if (!isExterior) {
		return std::nullopt;
	}

	if (currentSeason == SEASON::kNone) {
		UpdateSeason();
	}

	switch (currentSeason) {
	case SEASON::kWinter:
		return winter;
	case SEASON::kSpring:
		return spring;
	case SEASON::kSummer:
		return summer;
	case SEASON::kAutumn:
		return autumn;
	default:
		return std::nullopt;
	}
}

void SeasonManager::LoadMonthToSeasonMap(CSimpleIniA& a_ini)
{
	for (const auto& [month, monthName] : monthNames) {
		auto& [tes, irl] = monthName;
		INI::get_value(a_ini, monthToSeasons.at(month), "Settings", tes.data(),
			month == MONTH::kMorningStar ? ";0 - no season\n;1 - winter\n;2 - spring\n;3 - summer\n;4 - autumn\n\n;January" : irl.data());
	}
}

void SeasonManager::LoadSettings()
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(settings);

	logger::info("{:*^30}", "SETTINGS");

	//delete and recreate ini if new month-season settings are not found.
	if (const auto value = string::lexical_cast<std::int32_t>(ini.GetValue("Settings", "Morning Star", "-1")); value == -1) {
		ini.Delete("Settings", nullptr);
		ini.Delete("Winter", nullptr);
		ini.Delete("Spring", nullptr);
		ini.Delete("Summer", nullptr);
		ini.Delete("Autumn", nullptr);
	}

	INI::get_value(ini, seasonType, "Settings", "Season Type", ";0 - disabled\n;1 - permanent winter\n;2 - permanent spring\n;3 - permanent summer\n;4 - permanent autumn\n;5 - seasonal");

	logger::info("season type is {}", stl::to_underlying(seasonType));

	LoadMonthToSeasonMap(ini);

	winter.LoadSettings(ini, true);
	spring.LoadSettings(ini);
	summer.LoadSettings(ini);
	autumn.LoadSettings(ini);

	(void)ini.SaveFile(settings);
}

bool SeasonManager::ShouldRegenerateWinterFormSwap() const
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(serializedSeasonList);

	const auto& mods = RE::TESDataHandler::GetSingleton()->compiledFileCollection;
	const size_t actualModCount = mods.files.size() + mods.smallFiles.size();

	const auto expectedModCount = string::lexical_cast<size_t>(ini.GetValue("Game", "Mod Count", "0"));

	const auto shouldRegenerate = actualModCount != expectedModCount;

	if (shouldRegenerate) {
		ini.SetValue("Game", "Mod Count", std::to_string(actualModCount).c_str(), nullptr);
		if (expectedModCount != 0) {
			logger::info("Mod count has changed since last run ({} -> {}), regenerating main WIN formswap", expectedModCount, actualModCount);
		} else {
			logger::info("Regenerating main WIN formswap since last update");
		}
		(void)ini.SaveFile(serializedSeasonList);
	}

	return shouldRegenerate;
}

void SeasonManager::LoadOrGenerateWinterFormSwap()
{
	constexpr auto path = L"Data/Seasons/MainFormSwap_WIN.ini";

	logger::info("Loading main WIN formswap settings");

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.SetMultiKey();
	ini.SetAllowEmptyValues();

	ini.LoadFile(path);

	if (winter.GetFormSwapMap().GenerateFormSwaps(ini, ShouldRegenerateWinterFormSwap())) {
		(void)ini.SaveFile(path);
	}
}

void SeasonManager::LoadSeasonData(Season& a_season, CSimpleIniA& a_settings)
{
	std::vector<std::string> configs;

	const auto& [type, suffix] = a_season.GetID();

	for (constexpr auto folder = R"(Data\Seasons)"; const auto& entry : std::filesystem::directory_iterator(folder)) {
		if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini"sv) {
			if (const auto path = entry.path().string(); path.contains(suffix) && !path.contains("MainFormSwap"sv)) {
				configs.push_back(path);
			}
		}
	}

	if (configs.empty()) {
		logger::warn("No .ini files with _{} suffix were found in the Data/Seasons folder, skipping {} formswaps for {}...", suffix, suffix == "WIN" ? "secondary" : "all", type);
		return;
	}

	logger::info("{} matching inis found", configs.size());

	std::ranges::sort(configs);

	for (auto& path : configs) {
		logger::info("	INI : {}", path);

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetMultiKey();
		ini.SetAllowEmptyValues();

		if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
			logger::error("	couldn't read INI");
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

	LoadSeasonData(winter, settingsINI);
	LoadSeasonData(spring, settingsINI);
	LoadSeasonData(summer, settingsINI);
	LoadSeasonData(autumn, settingsINI);

	(void)settingsINI.SaveFile(settings);
}

void SeasonManager::SaveSeason(std::string_view a_savePath)
{
	if (const auto player = RE::PlayerCharacter::GetSingleton(); !player->parentCell || !player->parentCell->IsExteriorCell()) {
		return;
	}

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(serializedSeasonList);

	const auto season = GetCurrentSeason();
	currentSeason = season ? season->get().GetType() : SEASON::kNone;

	ini.SetValue("Saves", a_savePath.data(), std::to_string(stl::to_underlying(currentSeason)).c_str(), nullptr);

	(void)ini.SaveFile(serializedSeasonList);
}

void SeasonManager::LoadSeason(const std::string& a_savePath)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(serializedSeasonList);

	currentSeason = string::lexical_cast<SEASON>(ini.GetValue("Saves", a_savePath.c_str(), "3"));
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
		wchar_t* buffer{ nullptr };
		const auto result = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));
		std::unique_ptr<wchar_t[], decltype(&::CoTaskMemFree)> knownPath(buffer, ::CoTaskMemFree);
		if (!knownPath || result != S_OK) {
			logger::error("failed to get known folder path"sv);
			return std::nullopt;
		}

		std::filesystem::path path = knownPath.get();
#ifndef SKYRIMVR
		path /= "My Games/Skyrim Special Edition/"sv;
#else
		path /= "My Games/Skyrim VR/"sv;
#endif
		path /= RE::INISettingCollection::GetSingleton()->GetSetting("sLocalSavePath:General")->GetString();
		return path;
	};

	const auto directory = get_save_directory();
	if (!directory) {
		return;
	}

	logger::info("Save directory is {}", directory->string());

	CSimpleIniA ini;
	ini.SetUnicode();

	if (const auto rc = ini.LoadFile(serializedSeasonList); rc < 0) {
		return;
	}

	if (const auto values = ini.GetSection("Saves"); values && !values->empty()) {
		std::vector<std::string> badSaves;
		badSaves.reserve(values->size());
		for (const auto& key : *values | std::views::keys) {
			auto save = fmt::format("{}{}.ess", directory->string(), key.pItem);
			if (!std::filesystem::exists(save)) {
				badSaves.emplace_back(key.pItem);
			}
		}
		for (auto& badSave : badSaves) {
			ini.DeleteValue("Saves", badSave.c_str(), nullptr);
		}
	}

	(void)ini.SaveFile(serializedSeasonList);
}

SEASON SeasonManager::GetSeasonType()
{
	const auto season = GetSeason();
	return season ? season->get().GetType() : SEASON::kNone;
}

bool SeasonManager::CanApplySnowShader()
{
	const auto season = GetSeason();
	return season ? season->get().CanApplySnowShader() : false;
}

std::pair<bool, std::string> SeasonManager::CanSwapLOD(LOD_TYPE a_type)
{
	const auto season = GetSeason();
	return season ? std::make_pair(season->get().CanSwapLOD(a_type), season->get().GetID().suffix) : std::make_pair(false, "");
}

bool SeasonManager::CanSwapLandscape()
{
	const auto season = GetSeason();
	return season ? season->get().CanSwapLandscape() : false;
}

bool SeasonManager::CanSwapForm(RE::FormType a_formType)
{
	const auto season = GetSeason();
	return season ? season->get().CanSwapForm(a_formType) : false;
}

RE::TESBoundObject* SeasonManager::GetSwapForm(const RE::TESForm* a_form)
{
	const auto season = GetSeason();
	return season ? season->get().GetFormSwapMap().GetSwapForm(a_form) : nullptr;
}

RE::TESLandTexture* SeasonManager::GetSwapLandTexture(const RE::TESLandTexture* a_landTxst)
{
	const auto season = GetSeason();
	return season ? season->get().GetFormSwapMap().GetSwapLandTexture(a_landTxst) : nullptr;
}

RE::TESLandTexture* SeasonManager::GetSwapLandTextureFromTextureSet(const RE::BGSTextureSet* a_txst)
{
	const auto season = GetSeason();
	return season ? season->get().GetFormSwapMap().GetSwapLandTextureFromTextureSet(a_txst) : nullptr;
}

bool SeasonManager::GetUseAltGrass()
{
	const auto season = GetSeason();
	return season ? season->get().GetUseAltGrass() : false;
}

bool SeasonManager::GetExterior()
{
	return isExterior;
}

void SeasonManager::SetExterior(bool a_isExterior)
{
	isExterior = a_isExterior;
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
