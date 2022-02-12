#pragma once

#include "SeasonManager.h"

namespace FormSwap
{
	using FormSwapData = std::pair<RE::TESBoundObject*, bool>;

	struct util
	{
		static RE::TESBoundObject* get_original_base(RE::TESObjectREFR* a_ref)
		{
			const auto it = originals.find(a_ref->GetFormID());
			return it != originals.end() ? RE::TESForm::LookupByID<RE::TESBoundObject>(it->second) : a_ref->GetBaseObject();
		}

		static void set_original_base(const RE::TESObjectREFR* a_ref, const RE::TESBoundObject* a_originalBase)
		{
			originals.emplace(a_ref->GetFormID(), a_originalBase->GetFormID());
		}

		static FormSwapData get_form_swap(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_base)
		{
			const auto can_swap_base = [&]() {
				return !a_ref->IsDynamicForm() && !a_base->IsDynamicForm() && SeasonManager::GetSingleton()->CanSwapForm(a_base->GetFormType());
			};

			if (const auto origBase = a_ref && can_swap_base() ? get_original_base(a_ref) : nullptr) {
				const auto seasonManager = SeasonManager::GetSingleton();
				const auto replaceBase = seasonManager->GetSwapForm(origBase);

				if (replaceBase && seasonManager->GetSeasonType() == SEASON::kWinter && a_ref->IsInWater()) {
					return { replaceBase, true };
				}

				return { replaceBase, false };
			}
			return { nullptr, true };
		}

		static bool has_form_swap(RE::TESBoundObject* a_base)
		{
			return SeasonManager::GetSingleton()->GetSwapForm(a_base) != nullptr;
		}

		static bool can_apply_snow_shader(RE::TESObjectREFR* a_ref)
		{
			if (!SeasonManager::GetSingleton()->CanApplySnowShader()) {
				return false;
			}

			const auto base = get_original_base(a_ref);

			if (!base || has_form_swap(base) || base->IsNot(RE::FormType::Static, RE::FormType::MovableStatic, RE::FormType::Container) || base->IsMarker() || base->IsHeadingMarker()) {
				return false;
			}

			if (a_ref->IsInWater()) {
				return false;
			}

			if (const auto model = base->As<RE::TESModel>(); model && (model->model.empty() || std::ranges::any_of(snowShaderBlackList, [&](const auto str) {
					return string::icontains(model->model, str);
				}))) {
				return false;
			}

			if (const auto stat = base->As<RE::TESObjectSTAT>(); stat && (stat->HasTreeLOD() || stat->IsSkyObject() || stat->data.materialObj != nullptr)) {
				return false;
			}

			return true;
		}

	private:
		static inline std::array snowShaderBlackList = {
			R"(Effects\)"sv,
			"WetRocks"sv,
			"DynDOLOD"sv
		};

		static inline Map::FormID originals{};
	};

	struct GetHandle
	{
		static RE::RefHandle& thunk(RE::TESObjectREFR* a_ref, RE::RefHandle& a_handle)
		{
			if (const auto base = a_ref ? a_ref->GetBaseObject() : nullptr; a_ref && base) {
				const auto& [replaceBase, rejected] = util::get_form_swap(a_ref, base);

				if (replaceBase) {
					util::set_original_base(a_ref, base);
					if (!rejected) {
						a_ref->SetObjectReference(replaceBase);
					}
				} else if (!a_ref->IsDynamicForm()) {
					const auto origBase = util::get_original_base(a_ref);
					if (origBase && a_ref->GetBaseObject() != origBase) {
						a_ref->SetObjectReference(origBase);
					}
				}
			}

			return func(a_ref, a_handle);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct Load3D
	{
		static RE::NiAVObject* thunk(RE::TESObjectREFR* a_ref, bool a_backgroundLoading)
		{
			const auto node = func(a_ref, a_backgroundLoading);

			if (util::can_apply_snow_shader(a_ref) && node) {
				auto& [init, projectedParams, projectedColor] = defaultObj;
				if (!init) {
					const auto snowMat = RE::TESForm::LookupByEditorID<RE::BGSMaterialObject>("SnowMaterialObject1P");

					projectedColor = snowMat->directionalData.singlePassColor;
					projectedParams = RE::NiColorA{
						snowMat->directionalData.falloffScale,
						snowMat->directionalData.falloffBias,
						1.0f / snowMat->directionalData.noiseUVScale,
						std::cosf(RE::deg_to_rad(90.0f))
					};

					init = true;
				}
				if (node->SetProjectedUVData(projectedParams, projectedColor, true)) {
					if (const auto snowShaderData = RE::NiBooleanExtraData::Create("SOS_SNOW_SHADER", true)) {
						node->AddExtraData(snowShaderData);
					}
				}
			}

			return node;
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static inline constexpr std::size_t size = 0x6A;

	private:
		struct projectedUV
		{
			bool init{ false };
			RE::NiColorA projectedParams{};
			RE::NiColor projectedColor{};
		};

		static inline projectedUV defaultObj{};
	};

	inline void Install()
	{
		REL::Relocation<std::uintptr_t> model_loader_queue_ref{ REL::ID(12910) };
		stl::write_thunk_call<GetHandle>(model_loader_queue_ref.address() + 0x3E);

		stl::write_vfunc<RE::TESObjectREFR, Load3D>();
		logger::info("Installed form swapper"sv);
	}
}
