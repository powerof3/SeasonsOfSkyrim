#include "FormSwap.h"
#include "LODSwap.h"
#include "LandscapeSwap.h"
#include "MergeMapperPluginAPI.h"
#include "Papyrus.h"
#include "SeasonManager.h"
#include "SnowSwap.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case SKSE::MessagingInterface::kPostLoad:
		{
			try {
				SeasonManager::GetSingleton()->LoadSettings();
			} catch (...) {
				logger::error("Exception caught when loading settings! Check whether your setting values are valid. Default values will be used instead");
			}

			logger::info("{:*^30}", "HOOKS");

			SeasonManager::InstallHooks();

			FormSwap::Install();
			LandscapeSwap::Install();
			LODSwap::Install();
			SnowSwap::Install();
		}
		break;
	case SKSE::MessagingInterface::kPostPostLoad:
		{
			logger::info("{:*^30}", "MERGES");
			MergeMapperPluginAPI::GetMergeMapperInterface001();  // Request interface
			if (g_mergeMapperInterface) {                        // Use Interface
				const auto version = g_mergeMapperInterface->GetBuildNumber();
				logger::info("Got MergeMapper interface buildnumber {}", version);
			} else
				logger::info("MergeMapper not detected");
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			Cache::DataHolder::GetSingleton()->GetData();

			logger::info("{:*^30}", "CONFIG");

			std::filesystem::path seasonsPath{ "Data/Seasons"sv };
			if (std::filesystem::directory_entry seasonsFolder{ seasonsPath }; !seasonsFolder.exists()) {
				logger::info("Existing Seasons folder not found, creating it");
				std::filesystem::create_directory(seasonsPath);
			}

			SnowSwap::Manager::GetSingleton()->LoadSnowShaderSettings();

			const auto manager = SeasonManager::GetSingleton();
			manager->LoadOrGenerateWinterFormSwap();
			manager->LoadSeasonData();
			manager->RegisterEvents();
			manager->CleanupSerializedSeasonList();
		}
		break;
	case SKSE::MessagingInterface::kSaveGame:
		{
			std::string_view savePath = { static_cast<char*>(a_message->data), a_message->dataLen };
			SeasonManager::GetSingleton()->SaveSeason(savePath);
		}
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		{
			std::string savePath({ static_cast<char*>(a_message->data), a_message->dataLen });
			string::replace_last_instance(savePath, ".ess", "");

			SeasonManager::GetSingleton()->LoadSeason(savePath);
		}
		break;
	case SKSE::MessagingInterface::kDeleteGame:
		{
			std::string_view savePath = { static_cast<char*>(a_message->data), a_message->dataLen };
			SeasonManager::GetSingleton()->ClearSeason(savePath);
		}
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("Seasons Of Skyrim");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Seasons Of Skyrim";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver
#	ifndef SKYRIMVR
		< SKSE::RUNTIME_1_5_39
#	else
		> SKSE::RUNTIME_VR_1_4_15_1
#	endif
	) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}
#endif

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%l] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("loaded");

	SKSE::Init(a_skse);

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	const auto papyrus = SKSE::GetPapyrusInterface();
	papyrus->Register(Papyrus::Bind);

	return true;
}
