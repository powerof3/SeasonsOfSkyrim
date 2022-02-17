#pragma once

#include "SeasonManager.h"

namespace FormSwap
{
	struct detail
	{
		static std::pair<RE::TESBoundObject*, bool> get_form_swap(RE::TESObjectREFR* a_ref, const RE::TESBoundObject* a_base)
		{
			const auto seasonManager = SeasonManager::GetSingleton();

			if (const auto origBase = seasonManager->CanSwapForm(a_base->GetFormType()) ? util::get_original_base(a_ref) : nullptr; origBase) {
				return { seasonManager->GetSwapForm(origBase), seasonManager->GetSeasonType() == SEASON::kWinter && a_ref->IsInWater() };
			}

			return { nullptr, true };
		}
	};

	struct GetHandle
	{
		static RE::RefHandle& thunk(RE::TESObjectREFR* a_ref, RE::RefHandle& a_handle)
		{
			const auto base = a_ref && !a_ref->IsDynamicForm() && !a_ref->IsDeleted() ? a_ref->GetBaseObject() : nullptr;
			if (a_ref && base) {
				const auto& [replaceBase, rejected] = detail::get_form_swap(a_ref, base);

				if (replaceBase) {
					util::set_original_base(a_ref, base);
					if (!rejected) {
						a_ref->SetObjectReference(replaceBase);
					}
				} else {
					if (const auto origBase = util::get_original_base(a_ref); origBase && origBase != base) {
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
		REL::Relocation<std::uintptr_t> target{ REL::ID(12910) };  //ModelLoader::QueueReference
		stl::write_thunk_call<GetHandle>(target.address() + 0x3E);

		logger::info("Installed form swapper"sv);
	}
}
