#pragma once

#include "SeasonManager.h"

namespace FormSwap
{
	struct detail
	{
		static bool can_apply_snow_shader(const RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_base, RE::NiAVObject* a_node)
		{
			const auto seasonManager = SeasonManager::GetSingleton();
			if (!a_base || !a_node || seasonManager->GetSeasonType() != SEASON::kWinter) {
				return false;
			}

			if (a_base->IsNot(RE::FormType::Static, RE::FormType::MovableStatic, RE::FormType::Container) || a_ref->IsInWater()) {
				return false;
			}

			if (const auto model = a_base->As<RE::TESModel>(); model && (model->model.empty() || std::ranges::any_of(snowShaderBlackList, [&](const auto str) {
					return string::icontains(model->model, str);
				}))) {
				return false;
			}

			if (const auto stat = a_base->As<RE::TESObjectSTAT>(); stat && stat->data.materialObj) {
				return false;
			}

			return true;
		}

		static std::pair<RE::TESBoundObject*, bool> get_form_swap(const RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_base)
		{
			const auto seasonManager = SeasonManager::GetSingleton();
			const auto replaceBase = a_base && !a_base->IsDynamicForm() && seasonManager->IsSwapAllowed(a_base) ? seasonManager->GetSwapForm(a_base) : nullptr;

			if (replaceBase && SeasonManager::GetSingleton()->GetSeasonType() == SEASON::kWinter && a_ref->IsInWater()) {
				return { replaceBase, true };
			}

			return { replaceBase, false };
		}

	private:
		static inline std::array snowShaderBlackList = {
			R"(Effects\)"sv,
			R"(Sky\)"sv,
			"WetRocks"sv,
			"DynDOLOD"sv
		};
	};

	struct Load3D
	{
		static RE::NiAVObject* thunk(RE::TESObjectREFR* a_ref, bool a_backgroundLoading)
		{
			const auto base = a_ref->GetBaseObject();
			const auto& [replaceBase, rejected] = detail::get_form_swap(a_ref, base);

			if (replaceBase && !rejected) {
				a_ref->SetObjectReference(replaceBase);
			}

			const auto node = func(a_ref, a_backgroundLoading);

			if (!replaceBase && detail::can_apply_snow_shader(a_ref, base, node)) {
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
		stl::write_vfunc<RE::TESObjectREFR, Load3D>();
		logger::info("Installed form swapper"sv);
	}
}
