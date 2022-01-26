#include "FormSwap.h"
#include "LODSwap.h"
#include "LandscapeSwap.h"
#include "SeasonManager.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
		Cache::DataHolder::GetSingleton()->GetData();

		logger::info("{:*^30}", "FORM SWAP");

		const auto manager = SeasonManager::GetSingleton();
		manager->LoadOrGenerateWinterFormSwap();
		manager->LoadFormSwaps();
		manager->RegisterEvents();
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
	spdlog::set_pattern("%v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Seasons Of Skyrim";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("loaded plugin");

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(64);

	SeasonManager::GetSingleton()->LoadSettings();

	logger::info("{:*^30}", "HOOKS");

	FormSwap::Install();
	LandscapeSwap::Install();
	LODSwap::Install();
	SeasonManager::InstallHooks();

    const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	return true;
}
