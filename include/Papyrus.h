#pragma once

namespace Papyrus
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	bool Bind(VM* a_vm);

	namespace Seasons
	{
		std::uint32_t GetCurrentSeason(VM*, StackID, RE::StaticFunctionTag*);

		void Bind(VM& a_vm);
	}
}
