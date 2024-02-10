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

	struct GetHandle
	{
		static RE::RefHandle& thunk(RE::TESObjectREFR* a_ref, RE::RefHandle& a_handle)
		{
			if (!a_ref || a_ref->IsDynamicForm() || a_ref->IsDeleted() || a_ref->IsDisabled()) {
				return func(a_ref, a_handle);
			}

			if (const auto base = a_ref->GetBaseObject()) {
				if (const auto replaceBase = detail::get_form_swap(a_ref, base)) {
					util::set_original_base(a_ref, base);
					a_ref->SetObjectReference(replaceBase);
				} else if (const auto origBase = util::get_original_base(a_ref); origBase && origBase != base) {
					a_ref->SetObjectReference(origBase);
				}
			}

			return func(a_ref, a_handle);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	inline void Install()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(12910, 13057), 0x3E };  //ModelLoader::QueueReference
		stl::write_thunk_call<GetHandle>(target.address());

		logger::info("Installed form swapper"sv);
	}
}
