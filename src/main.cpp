#include "FormSwap.h"
#include "LODSwap.h"
#include "LandscapeSwap.h"
#include "MergeMapper.h"
#include "SnowSwap.h"
#include "SeasonManager.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case SKSE::MessagingInterface::kPostLoad:
		{
			SeasonManager::GetSingleton()->LoadSettings();

			logger::info("{:*^30}", "HOOKS");

			SeasonManager::InstallHooks();

			FormSwap::Install();
			LandscapeSwap::Install();
			LODSwap::Install();
			SnowSwap::Install();
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

		    SnowSwap::Manager::GetSingleton()->LoadSnowShaderBlacklist();

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

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S] [%l] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Seasons Of Skyrim";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver <
#ifndef SKYRIMVR
		SKSE::RUNTIME_1_5_39
#else
		SKSE::RUNTIME_VR_1_4_15
#endif
	) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("loaded plugin");

	SKSE::Init(a_skse);
	MergeMapper::GetMerges();

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	return true;
}
