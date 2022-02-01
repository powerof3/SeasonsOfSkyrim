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

			if (a_base->IsNot(RE::FormType::Activator, RE::FormType::Container, RE::FormType::Furniture, RE::FormType::MovableStatic, RE::FormType::Static)) {
				return false;
			}

			if (!seasonManager->IsSwapAllowed(a_base) || a_base->IsMarker() || a_base->IsHeadingMarker() || a_base->IsWater()) {
				return false;
			}

			if (a_base->Is(RE::FormType::Activator, RE::FormType::Furniture) && a_node->HasAnimation()) {  // no grindstones, mills
				return false;
			}

			const auto waterLevel = a_ref->GetSubmergedWaterLevel(a_ref->GetPositionZ(), a_ref->GetParentCell());
			if (waterLevel >= 0.01f) {
				return false;
			}

			if (const auto model = a_base->As<RE::TESModel>(); model) {
				if (const std::string path = model->model.c_str(); path.empty() || std::ranges::any_of(snowShaderBlackList, [&](const auto str) { return string::icontains(path, str); })) {
					return false;
				}
			}

			if (const auto stat = a_base->As<RE::TESObjectSTAT>(); stat && stat->HasTreeLOD()) {
				return false;
			}

			return true;
		}

		static bool can_swap_static(const RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_base)
		{
			const auto seasonManager = SeasonManager::GetSingleton();
			if (!a_base || a_base->IsDynamicForm() || !seasonManager->IsSwapAllowed()) {
				return false;
			}

			if (seasonManager->GetSeasonType() != SEASON::kWinter) {
				const auto waterLevel = a_ref->GetSubmergedWaterLevel(a_ref->GetPositionZ(), a_ref->GetParentCell());
				return waterLevel >= 0.01f;
			}

			return true;
		}

	private:
		static inline std::array snowShaderBlackList = {
			R"(Effects\)"sv,
			R"(Sky\)"sv,
			"Marker"sv,
			"WetRocks"sv
		};
	};

	struct Load3D
	{
		static RE::NiAVObject* thunk(RE::TESObjectREFR* a_ref, bool a_backgroundLoading)
		{
			const auto base = a_ref->GetBaseObject();
			const auto replaceBase = detail::can_swap_static(a_ref, base) ? SeasonManager::GetSingleton()->GetSwapForm(base) : nullptr;

			if (replaceBase) {
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
					if (const auto snowShaderData = RE::NiBooleanExtraData::Create("SOS_SNOW_SHADER", true); snowShaderData) {
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
