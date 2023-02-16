#pragma once

namespace Papyrus
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	bool Bind(VM* a_vm);

	namespace Events
	{
		class Manager
		{
		public:
			[[nodiscard]] static Manager* GetSingleton()
			{
				static Manager singleton;
				return &singleton;
			}

			enum : std::uint32_t
			{
				kSeasonChange = 'SOSC'
			};

			SKSE::RegistrationSet<std::uint32_t, std::uint32_t, bool> seasonChange{ "OnSeasonChange"sv };

			void Save(SKSE::SerializationInterface* a_intfc, std::uint32_t a_version);
			void Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type);
			void Revert(SKSE::SerializationInterface* a_intfc);

		private:
			Manager() = default;
			Manager(const Manager&) = delete;
			Manager(Manager&&) = delete;
			~Manager() = default;

			Manager& operator=(const Manager&) = delete;
			Manager& operator=(Manager&&) = delete;
		};
	}

	namespace Functions
	{
		template <class T>
		void RegisterForSeasonChangeImpl(T* a_type)
		{
			if (a_type) {
				Events::Manager::GetSingleton()->seasonChange.Register(a_type);
			}
		}

		template <class T>
		void UnregisterForSeasonChangeImpl(T* a_type)
		{
			if (a_type) {
				Events::Manager::GetSingleton()->seasonChange.Unregister(a_type);
			}
		}

		void RegisterForSeasonChange_Alias(VM*, StackID, RE::StaticFunctionTag*, RE::BGSRefAlias* a_alias);
		void RegisterForSeasonChange_AME(VM*, StackID, RE::StaticFunctionTag*, RE::ActiveEffect* a_activeEffect);
		void RegisterForSeasonChange_Form(VM*, StackID, RE::StaticFunctionTag*, RE::TESForm* a_form);

		void UnregisterForSeasonChange_Alias(VM*, StackID, RE::StaticFunctionTag*, RE::BGSRefAlias* a_alias);
		void UnregisterForSeasonChange_AME(VM*, StackID, RE::StaticFunctionTag*, RE::ActiveEffect* a_activeEffect);
		void UnregisterForSeasonChange_Form(VM*, StackID, RE::StaticFunctionTag*, RE::TESForm* a_form);

		std::uint32_t GetCurrentSeason(VM*, StackID, RE::StaticFunctionTag*);

		std::uint32_t GetSeasonOverride(VM*, StackID, RE::StaticFunctionTag*);
		void          SetSeasonOverride(VM*, StackID, RE::StaticFunctionTag*, std::uint32_t a_season);
		void          ClearSeasonOverride(VM*, StackID, RE::StaticFunctionTag*);

		void Bind(VM& a_vm);
	}
}
