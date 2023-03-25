#pragma once

#include "SeasonManager.h"

namespace Debug
{
	namespace detail
	{
		constexpr auto LONG_NAME = "GetLandTexture"sv;
		constexpr auto SHORT_NAME = "GLT"sv;

		[[nodiscard]] const std::string& HelpString()
		{
			static auto help = []() {
				std::string buf;
				buf += "Get Land Texture\n";
				return buf;
			}();
			return help;
		}

		bool Execute(const RE::SCRIPT_PARAMETER*, RE::SCRIPT_FUNCTION::ScriptData*, RE::TESObjectREFR*, RE::TESObjectREFR*, RE::Script*, RE::ScriptLocals*, double&, std::uint32_t&)
		{
			constexpr auto print = [](const char* a_fmt) {
				if (RE::ConsoleLog::IsConsoleMode()) {
					RE::ConsoleLog::GetSingleton()->Print(a_fmt);
				}
			};

			if (const auto LT = RE::TES::GetSingleton()->GetLandTexture(RE::PlayerCharacter::GetSingleton()->GetPosition())) {
				const auto swappedLT = SeasonManager::GetSingleton()->GetSwapLandTexture(LT);
				if (swappedLT) {
					print(fmt::format("{} -> {}", util::get_editorID(LT), util::get_editorID(swappedLT)).c_str());
				} else {
					print(util::get_editorID(LT).c_str());
				}
			} else {
				print("no land texture at player pos");
			}

			return true;
		}
	}

	void Install()
	{
		if (const auto function = RE::SCRIPT_FUNCTION::LocateConsoleCommand("SetStackDepth"); function) {
			function->functionName = detail::LONG_NAME.data();
			function->shortName = detail::SHORT_NAME.data();
			function->helpString = detail::HelpString().data();
			function->referenceFunction = false;
			function->SetParameters();
			function->executeFunction = &detail::Execute;
			function->conditionFunction = nullptr;

			logger::debug("installed {}", detail::LONG_NAME);
		}
	}
}
