#pragma once

namespace Papyrus
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	bool Bind(VM* a_vm);

	namespace Seasons
	{
		std::uint32_t GetCurrentSeason(VM*, StackID, RE::StaticFunctionTag*);

		bool GetSeasonOverride(VM*, StackID, RE::StaticFunctionTag*, std::uint32_t a_season);
	    void SetSeasonOverride(VM*, StackID, RE::StaticFunctionTag*, std::uint32_t a_season);
		void ClearSeasonOverride(VM*, StackID, RE::StaticFunctionTag*, std::uint32_t a_season);

		void Bind(VM& a_vm);
	}
}
