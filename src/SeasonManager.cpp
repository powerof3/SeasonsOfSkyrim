#include "SeasonManager.h"

std::optional<Season> SeasonManager::GetSeason()
{
	switch (seasonType) {
	case SEASON_TYPE::kOff:
		return std::nullopt;
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
			using MONTH = RE::Calendar::Month;

			switch (const auto calendar = RE::Calendar::GetSingleton(); calendar ? calendar->GetMonth() : 7) {
			case MONTH::kEveningStar:
			case MONTH::kSunsDawn:
			case MONTH::kMorningStar:
				return winter;
			case MONTH::kFirstSeed:
			case MONTH::kRainsHand:
			case MONTH::kSecondSeed:
				return spring;
			case MONTH::kMidyear:
			case MONTH::kSunsHeight:
			case MONTH::kLastSeed:
				return summer;
			case MONTH::kHearthfire:
			case MONTH::kFrostfall:
			case MONTH::kSunsDusk:
				return autumn;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}

	return std::nullopt;
}

void SeasonManager::LoadSettings()
{
	constexpr auto path = L"Data/SKSE/Plugins/po3_SeasonsOfSkyrim.ini";

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	INI::get_value(ini, seasonType, "Settings", "Season Type", ";0 - disabled\n;1 - permanent winter\n;2 - permanent spring\n;3 - permanent summer\n;4 - permanent autumn\n;5 - seasonal");

	winter.LoadSettingsAndVerify(ini);
	spring.LoadSettingsAndVerify(ini);
	summer.LoadSettingsAndVerify(ini);
	autumn.LoadSettingsAndVerify(ini);

	(void)ini.SaveFile(path);
}

void SeasonManager::LoadOrGenerateWinterFormSwap()
{
	constexpr auto path = L"Data/Seasons/MainFormSwap_WIN.ini";

	logger::info("loading main formswap settings");

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.SetMultiKey();
	ini.SetAllowEmptyValues();

	ini.LoadFile(path);

	if (winter.GetFormSwapMap().GenerateFormSwaps(ini)) {
		ini.SaveFile(path);
	}
}

void SeasonManager::LoadFormSwaps_Impl(Season& a_season)
{
	std::vector<std::string> configs;

    const auto& ID = a_season.GetID();

    for (constexpr auto folder = R"(Data\Seasons)"; const auto& entry : std::filesystem::directory_iterator(folder)) {
		if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini"sv) {
			if (const auto path = entry.path().string(); path.rfind(ID) != std::string::npos && path.find("MainFormSwap") == std::string::npos) {
				configs.push_back(path);
			}
		}
	}

	if (configs.empty()) {
		logger::warn("No .ini files with _{} suffix were found in the Data/Seasons folder, skipping formswaps for {}...", ID, a_season.GetType());
		return;
	}

	logger::info("{} matching inis found", configs.size());

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

		a_season.GetFormSwapMap().LoadFormSwaps(ini);
	}
}

void SeasonManager::LoadFormSwaps()
{
	LoadFormSwaps_Impl(winter);
	LoadFormSwaps_Impl(spring);
	LoadFormSwaps_Impl(summer);
	LoadFormSwaps_Impl(autumn);
}

bool SeasonManager::CanSwapGrass()
{
	const auto season = GetSeason();
	return season ? season->CanSwapGrass() : false;
}

std::pair<bool, std::string> SeasonManager::CanSwapLOD()
{
	const auto season = GetSeason();
	return season ? std::make_pair(season->CanSwapLOD(), season->GetID()) : std::make_pair(false, "");
}

bool SeasonManager::IsLandscapeSwapAllowed()
{
	const auto season = GetSeason();
	return season ? season->IsLandscapeSwapAllowed() : false;
}

bool SeasonManager::IsSwapAllowed()
{
	const auto season = GetSeason();
	return season ? !isExterior && season->IsSwapAllowed() : false;
}

bool SeasonManager::IsSwapAllowed(const RE::TESForm* a_form)
{
	const auto season = GetSeason();
	return season ? !isExterior && season->IsSwapAllowed(a_form) : false;
}

RE::TESBoundObject* SeasonManager::GetSwapForm(const RE::TESForm* a_form)
{
	auto season = GetSeason();
	return season ? season->GetFormSwapMap().GetSwapForm(a_form) : nullptr;
}

RE::TESLandTexture* SeasonManager::GetLandTexture(const RE::TESForm* a_form)
{
	auto season = GetSeason();
	return season ? season->GetFormSwapMap().GetLandTexture(a_form) : nullptr;
}

RE::TESLandTexture* SeasonManager::GetLandTextureFromTextureSet(const RE::TESForm* a_form)
{
	auto season = GetSeason();
	return season ? season->GetFormSwapMap().GetLandTextureFromTextureSet(a_form) : nullptr;
}
