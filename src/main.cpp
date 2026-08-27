#include "Debug.h"
#include "FormSwap.h"
#include "LODSwap.h"
#include "LandscapeSwap.h"
#include "Papyrus.h"
#include "SeasonManager.h"
#include "SnowSwap.h"

REL::Version gameVersion{};

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case SKSE::MessagingInterface::kPostLoad:
		{
			try {
				SeasonManager::GetSingleton()->LoadSettings();
			} catch (...) {
				REX::ERROR("Exception caught when loading settings! Check whether your setting values are valid. Default values will be used instead");
			}

			REX::INFO("{:*^30}", "HOOKS");

			SeasonManager::InstallHooks();

			FormSwap::Install();
			LandscapeSwap::Install();
			SnowSwap::Install();

			Debug::Install();
		}
		break;
	case SKSE::MessagingInterface::kPostPostLoad:
		{
			REX::INFO("{:*^30}", "MERGES");
			MergeMapperPluginAPI::GetMergeMapperInterface001();
			if (g_mergeMapperInterface) {
				const auto version = g_mergeMapperInterface->GetBuildNumber();
				REX::INFO("Got MergeMapper interface buildnumber {}", version);
			} else {
				REX::INFO("MergeMapper not detected");
			}
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			REX::INFO("{:*^30}", "DEPENDENCIES");

			auto tweaks = GetModuleHandle(L"po3_Tweaks");
			REX::INFO("powerofthree's Tweaks (po3_tweaks) detected : {}", tweaks != nullptr);

			std::string tweaksError{};
			if (tweaks == nullptr) {
				tweaksError = std::format("powerofthree's Tweaks is not installed! Please check if you have installed the correct version for your game ({}) if you have done so already.\n", gameVersion);
			}
			std::string sosESPError{};
			const auto  dataHandler = RE::TESDataHandler::GetSingleton();
			if (!dataHandler->LookupLoadedLightModByName("SnowOverSkyrim.esp") && !dataHandler->LookupLoadedModByName("SnowOverSkyrim.esp") && !RE::TESForm::LookupByEditorID<RE::BGSMaterialObject>("SOS_WIN_SnowMaterialObjectSP")) {
				sosESPError = "SnowOverSkyrim.esp is not enabled!\n";
			}
			if (!tweaksError.empty() || !sosESPError.empty()) {
				std::string error{ "[Seasons of Skyrim] Missing dependencies! This mod may not work as expected without them.\n\n" };
				error.append(tweaksError).append(sosESPError);
				RE::DebugMessageBox(error.c_str());
				RE::ConsoleLog::GetSingleton()->Print(error.c_str());
			}

			Cache::DataHolder::GetSingleton()->GetData();

			REX::INFO("{:*^30}", "CONFIG");

			const std::filesystem::path seasonsPath{ "Data/Seasons"sv };
			if (std::filesystem::directory_entry seasonsFolder{ seasonsPath }; !seasonsFolder.exists()) {
				REX::INFO("Existing Seasons folder not found, creating it");
				std::filesystem::create_directory(seasonsPath);
			}

			SnowSwap::Manager::GetSingleton()->LoadSnowShaderSettings();

			const auto manager = SeasonManager::GetSingleton();
			manager->LoadOrGenerateWinterFormSwap();
			manager->LoadSeasonData();
			manager->LoadValidWorldspaces();

			manager->CheckLODExists();
			LODSwap::Install();

			manager->RegisterEvents();
			manager->CleanupSerializedSeasonList();
		}
		break;
	case SKSE::MessagingInterface::kSaveGame:
		{
			std::string_view savePath{ static_cast<char*>(a_message->data), a_message->dataLen };
			SeasonManager::GetSingleton()->SaveSeason(savePath);
		}
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		{
			std::string savePath{ static_cast<char*>(a_message->data), a_message->dataLen };
			REX::STR::REPLACE_LAST_INSTANCE(savePath, ".ess", "");

			SeasonManager::GetSingleton()->LoadSeason(savePath);
		}
		break;
	case SKSE::MessagingInterface::kDeleteGame:
		{
			std::string_view savePath{ static_cast<char*>(a_message->data), a_message->dataLen };
			SeasonManager::GetSingleton()->ClearSeason(savePath);
		}
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_SUPPORT_AE
SKSE_PLUGIN_VERSION = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(REL::Version{ Version::MAJOR, Version::MINOR, Version::PATCH });
	v.PluginName("Seasons of Skyrim");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST });

	if constexpr (SKSE::RUNTIME_SSE_LATEST < Runtime::MIN_ADDRESS_LIBRARY_V5) {
		v.MinimumRequiredXSEVersion(REL::Version{ 2, 2, 5 });
	} else {
		v.MinimumRequiredXSEVersion(REL::Version{ 2, 3, 0 });
	}

	return v;
}();
#else
SKSE_PLUGIN_QUERY(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Sandbox When Idle";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		REX::CRITICAL("Loaded in editor, marking as incompatible");
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver
#	ifndef SKYRIMVR
		< SKSE::RUNTIME_SSE_1_5_39
#	else
		> SKSE::RUNTIME_VR_1_4_15_1
#	endif
	) {
		REX::CRITICAL("Unsupported runtime version {}", ver.string());
		return false;
	}

	return true;
}
#endif

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse, { .log = true,
						   .logName = Version::PROJECT.data(),
						   .trampoline = true,
						   .trampolineSize = 14 * 11 });

	auto runtimeVersion = a_skse->RuntimeVersion();

	REX::INFO("Game version : {}", runtimeVersion);

#ifdef SKYRIM_SUPPORT_AE
	if constexpr (SKSE::RUNTIME_SSE_LATEST < Runtime::MIN_ADDRESS_LIBRARY_V5) {
		if (runtimeVersion >= Runtime::MIN_ADDRESS_LIBRARY_V5) {
			REX::FAIL(
				"You are using a newer version of Skyrim than this version of {0} supports.\n"
				"Install the correct version of {0} for your game version.\n"
				"Runtime: {1}\n"
				"Supported: 1.6.1170 (Steam) / 1.6.1179 (GOG)",
				Version::PROJECT, runtimeVersion);
		}
	}
#endif

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	const auto papyrus = SKSE::GetPapyrusInterface();
	papyrus->Register(Papyrus::Bind);

	const auto serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID(Papyrus::Events::kSeasonsOfSkyrim);
	serialization->SetSaveCallback(Papyrus::Events::SaveCallback);
	serialization->SetLoadCallback(Papyrus::Events::LoadCallback);
	serialization->SetRevertCallback(Papyrus::Events::RevertCallback);

	return true;
}

extern "C" DLLEXPORT std::uint32_t GetCurrentSeason()
{
	return std::to_underlying(SeasonManager::GetSingleton()->GetCurrentSeasonType());
}

extern "C" DLLEXPORT std::uint32_t GetSeasonOverride()
{
	return std::to_underlying(SeasonManager::GetSingleton()->GetSeasonOverride());
}

extern "C" DLLEXPORT void SetSeasonOverride(std::uint32_t a_season)
{
	SeasonManager::GetSingleton()->SetSeasonOverride(static_cast<SEASON_TYPE>(a_season));
}

extern "C" DLLEXPORT void ClearSeasonOverride()
{
	SeasonManager::GetSingleton()->SetSeasonOverride(SEASON_TYPE::kNone);
}
