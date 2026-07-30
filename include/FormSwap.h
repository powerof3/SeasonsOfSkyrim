#pragma once

#include "SeasonManager.h"

namespace FormSwap
{
	struct GetHandle
	{
		static RE::RefHandle& thunk(RE::TESObjectREFR* a_ref, RE::RefHandle& a_handle)
		{
			if (!a_ref || a_ref->IsDynamicForm() || a_ref->IsDeleted() || a_ref->IsDisabled()) {
				return func(a_ref, a_handle);
			}

			if (const auto currentBase = a_ref->GetBaseObject()) {
				if (const auto origBase = Cache::get_original_base(a_ref)) {
					if (const auto swapBase = SeasonManager::GetSingleton()->GetSwapForm(origBase)) {
						if (swapBase != currentBase) {
							Cache::set_original_base(a_ref, currentBase);
							a_ref->SetObjectReference(swapBase);
						}
					} else if (origBase != currentBase) {
						a_ref->SetObjectReference(origBase);
					}
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
