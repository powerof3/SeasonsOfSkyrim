#pragma once

#include "SeasonManager.h"

namespace FormSwap
{
	struct detail
	{
		static RE::TESBoundObject* get_form_swap(RE::TESObjectREFR* a_ref, const RE::TESBoundObject* a_base)
		{
			const auto seasonMngr = SeasonManager::GetSingleton();
			const auto origBase = seasonMngr->CanSwapForm(a_base->GetFormType()) ? util::get_original_base(a_ref) : nullptr;

			return origBase ? seasonMngr->GetSwapForm(origBase) : nullptr;
		}
	};

	// no actors/projectiles
	// doesn't fire twice for same ref
    struct ShouldBackgroundClone
	{
		static bool thunk(RE::TESObjectREFR* a_this)
		{
			if (a_this->IsDynamicForm() || a_this->IsDeleted() || a_this->IsDisabled()) {
				return func(a_this);
			}

			if (const auto base = a_this->GetBaseObject()) {
				if (const auto replaceBase = detail::get_form_swap(a_this, base)) {
					util::set_original_base(a_this, base);
					a_this->SetObjectReference(replaceBase);
				} else if (const auto origBase = util::get_original_base(a_this); origBase && origBase != base) {
					a_this->SetObjectReference(origBase);
				}
			}

			return func(a_this);
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static inline constexpr std::size_t index{ 0 };
		static inline constexpr std::size_t size{ 0x6D };
	};

	inline void Install()
	{
		stl::write_vfunc<RE::TESObjectREFR, ShouldBackgroundClone>();

		logger::info("Installed form swapper"sv);
	}
}
