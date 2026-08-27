#include "Papyrus.h"
#include "SeasonManager.h"

namespace Papyrus
{
	bool Bind(VM* a_vm)
	{
		if (!a_vm) {
			REX::CRITICAL("couldn't get VM State"sv);
			return false;
		}

		REX::INFO("{:*^30}", "FUNCTIONS"sv);

		Functions::Bind(*a_vm);

		return true;
	}

	namespace Functions
	{
		void RegisterForSeasonChange_Alias(VM*, StackID, RE::StaticFunctionTag*, RE::BGSRefAlias* a_alias)
		{
			RegisterForSeasonChangeImpl(a_alias);
		}
		void RegisterForSeasonChange_AME(VM*, StackID, RE::StaticFunctionTag*, RE::ActiveEffect* a_activeEffect)
		{
			RegisterForSeasonChangeImpl(a_activeEffect);
		}
		void RegisterForSeasonChange_Form(VM*, StackID, RE::StaticFunctionTag*, RE::TESForm* a_form)
		{
			RegisterForSeasonChangeImpl(a_form);
		}

		void UnregisterForSeasonChange_Alias(VM*, StackID, RE::StaticFunctionTag*, RE::BGSRefAlias* a_alias)
		{
			UnregisterForSeasonChangeImpl(a_alias);
		}
		void UnregisterForSeasonChange_AME(VM*, StackID, RE::StaticFunctionTag*, RE::ActiveEffect* a_activeEffect)
		{
			UnregisterForSeasonChangeImpl(a_activeEffect);
		}
		void UnregisterForSeasonChange_Form(VM*, StackID, RE::StaticFunctionTag*, RE::TESForm* a_form)
		{
			UnregisterForSeasonChangeImpl(a_form);
		}

		std::uint32_t GetCurrentSeason(VM*, StackID, RE::StaticFunctionTag*)
		{
			return std::to_underlying(SeasonManager::GetSingleton()->GetCurrentSeasonType());
		}

		std::uint32_t GetSeasonOverride(VM*, StackID, RE::StaticFunctionTag*)
		{
			return std::to_underlying(SeasonManager::GetSingleton()->GetSeasonOverride());
		}
		void SetSeasonOverride(VM*, StackID, RE::StaticFunctionTag*, std::uint32_t a_season)
		{
			SeasonManager::GetSingleton()->SetSeasonOverride(static_cast<SEASON_TYPE>(a_season));
		}
		void ClearSeasonOverride(VM*, StackID, RE::StaticFunctionTag*)
		{
			SeasonManager::GetSingleton()->SetSeasonOverride(SEASON_TYPE::kNone);
		}

		void Bind(VM& a_vm)
		{
			constexpr auto script = "SeasonsOfSkyrim"sv;

			a_vm.RegisterFunction("GetCurrentSeason", script, GetCurrentSeason, true);

			a_vm.RegisterFunction("GetSeasonOverride", script, GetSeasonOverride);
			a_vm.RegisterFunction("SetSeasonOverride", script, SetSeasonOverride);
			a_vm.RegisterFunction("ClearSeasonOverride", script, ClearSeasonOverride);

			a_vm.RegisterFunction("RegisterForSeasonChange_Alias", script, RegisterForSeasonChange_Alias);
			a_vm.RegisterFunction("RegisterForSeasonChange_AME", script, RegisterForSeasonChange_AME);
			a_vm.RegisterFunction("RegisterForSeasonChange_Form", script, RegisterForSeasonChange_Form);

			a_vm.RegisterFunction("UnregisterForSeasonChange_Alias", script, UnregisterForSeasonChange_Alias);
			a_vm.RegisterFunction("UnregisterForSeasonChange_AME", script, UnregisterForSeasonChange_AME);
			a_vm.RegisterFunction("UnregisterForSeasonChange_Form", script, UnregisterForSeasonChange_Form);

			REX::INFO("Registered season functions"sv);
		}
	}

	namespace Events
	{
		void Manager::Save(SKSE::SerializationInterface* a_intfc, std::uint32_t a_version)
		{
			seasonChange.Save(a_intfc, kSeasonChange, a_version);
		}

		void Manager::Load(SKSE::SerializationInterface* a_intfc)
		{
			std::uint32_t type;
			std::uint32_t version;
			std::uint32_t length;
			while (a_intfc->GetNextRecordInfo(type, version, length)) {
				if (version != kSerializationVersion) {
					continue;
				}
				if (type == kSeasonChange) {
					seasonChange.Load(a_intfc);
				}
			}
		}

		void Manager::Revert(SKSE::SerializationInterface* a_intfc)
		{
			seasonChange.Revert(a_intfc);
		}

		void SaveCallback(SKSE::SerializationInterface* a_intfc)
		{
			Manager::GetSingleton()->Save(a_intfc, kSerializationVersion);
		}

		void LoadCallback(SKSE::SerializationInterface* a_intfc)
		{
			Manager::GetSingleton()->Load(a_intfc);
		}

		void RevertCallback(SKSE::SerializationInterface* a_intfc)
		{
			Manager::GetSingleton()->Revert(a_intfc);
		}
	}
}
