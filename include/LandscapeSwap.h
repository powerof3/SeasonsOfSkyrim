#pragma once

#include "SeasonManager.h"

namespace LandscapeSwap
{
	namespace Texture
	{
		struct GetAsShaderTextureSet
		{
			static RE::BSTextureSet* thunk(RE::BGSTextureSet* a_textureSet)
			{
				if (const auto seasonManager = SeasonManager::GetSingleton(); seasonManager->CanSwapLandscape()) {
					const auto newLandTexture = seasonManager->GetSwapLandTextureFromTextureSet(a_textureSet);
					return newLandTexture ? newLandTexture->textureSet : a_textureSet;
				}
				return a_textureSet;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		inline void Install()
		{
			REL::Relocation<std::uintptr_t> create_land_geometry{ REL::ID(18368) };
			constexpr std::array<std::uint64_t, 3> offsets{ 0x172, 0x18B, 0x1E6 };

			for (const auto& offset : offsets) {
				stl::write_thunk_call<GetAsShaderTextureSet>(create_land_geometry.address() + offset);
			}
		}
	}

	namespace Grass
	{
		namespace Standard
		{
			struct GetGrassList
			{
				static RE::BSSimpleList<RE::TESGrass*>& func(RE::TESLandTexture* a_landTexture)
				{
					if (const auto seasonManager = SeasonManager::GetSingleton(); seasonManager->CanSwapForm(RE::FormType::Grass)) {
						const auto swapLandTexture = seasonManager->GetSwapLandTexture(a_landTexture);

						return swapLandTexture ? swapLandTexture->textureGrassList : a_landTexture->textureGrassList;
					}
					return a_landTexture->textureGrassList;
				}

				static inline size_t size = 0x5;
				static inline std::uint64_t id = 18414;
			};

			inline void Install()
			{
				stl::asm_replace<GetGrassList>();
			}
		}

		namespace Alt
		{
			struct LoadGrassType
			{
				static RE::BSInstanceTriShape* thunk(RE::BGSGrassManager* a_grassManager, RE::GrassParam* a_param, std::int32_t a_arg3, std::int32_t a_arg4, std::uint64_t& a_grassTypeKey, RE::BSFixedString& grassModelKey)
				{
					const auto seasonManager = SeasonManager::GetSingleton();

					const auto grass = seasonManager->CanSwapForm(RE::FormType::Grass) ? RE::TESForm::LookupByID(a_param->grassFormID) : nullptr;
					const auto swapGrass = grass ? seasonManager->GetSwapForm<RE::TESGrass>(grass) : nullptr;

					if (swapGrass) {
						a_param->modelName = swapGrass->GetModel();
						a_param->grassFormID = swapGrass->GetFormID();
						a_param->heightRange = swapGrass->GetHeightRange();
						a_param->positionRange = swapGrass->GetPositionRange();
						a_param->colorRange = swapGrass->GetColorRange();
						a_param->wavePeriod = swapGrass->GetWavePeriod();
						a_param->hasVertexLighting = swapGrass->GetVertexLighting();
						a_param->hasUniformScaling = swapGrass->GetUniformScaling();
						a_param->fitsToSlope = swapGrass->GetFitToSlope();
					}

					return func(a_grassManager, a_param, a_arg3, a_arg4, a_grassTypeKey, grassModelKey);
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install()
			{
				constexpr std::array targets{
					std::make_pair(15204, 0x2F5),  // add_cell_grass
					std::make_pair(15205, 0x62B),  // add_cell_grass_from_buffer
					std::make_pair(15206, 0x25C),  // add_cell_grass_from_file
				};

				for (const auto& [id, offset] : targets) {
					REL::Relocation<std::uintptr_t> target{ REL::ID(id), offset };
					stl::write_thunk_call<LoadGrassType>(target.address());
				}
			}
		}

		inline void Install()
		{
			if (SeasonManager::GetSingleton()->GetUseAltGrass()) {
				logger::info("Using alternate grass generation");
				Alt::Install();
			} else {
				logger::info("Using standard grass generation");
				Standard::Install();
			}
		}
	}

	namespace Material
	{
		struct GetHavokMaterialType
		{
			static RE::MATERIAL_ID func(const RE::TESLandTexture* a_landTexture)
			{
				if (const auto seasonManager = SeasonManager::GetSingleton(); seasonManager->CanSwapLandscape()) {
					const auto newLandTexture = seasonManager->GetSwapLandTexture(a_landTexture);
					const auto materialType = newLandTexture ? newLandTexture->materialType : a_landTexture->materialType;

					return materialType ? materialType->materialID : RE::MATERIAL_ID::kNone;
				}
				return a_landTexture->materialType ? a_landTexture->materialType->materialID : RE::MATERIAL_ID::kNone;
			}

			static inline size_t size = 0xE;
			static inline std::uint64_t id = 18418;
		};

		inline void Install()
		{
			stl::asm_replace<GetHavokMaterialType>();
		}
	}

	inline void Install()
	{
		Texture::Install();
		Grass::Install();
		Material::Install();
	}
}
