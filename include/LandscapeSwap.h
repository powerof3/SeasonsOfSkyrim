#pragma once

#include "SeasonManager.h"

namespace LandscapeSwap
{
	namespace Texture
	{
		static inline REL::Relocation<std::uintptr_t> create_land_geometry{ RELOCATION_ID(18368, 18791) };

		struct IsConsideredSnow
		{
			static float thunk(const RE::TESLandTexture* a_LT)
			{
				const auto manager = SeasonManager::GetSingleton();

				const auto swapLT = manager->CanSwapLandscape() ? manager->GetSwapLandTexture(a_LT) : a_LT;
				return swapLT ? swapLT->shaderTextureIndex != 0 : a_LT->shaderTextureIndex != 0;
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static void Install()
			{
				constexpr std::array<std::uint64_t, 2> offsets{
				    OFFSET(0x155,0x157),
				    0x1C9
				};

			    for (const auto& offset : offsets) {
					stl::write_thunk_call<IsConsideredSnow>(create_land_geometry.address() + offset);
				}
			}
		};

		struct GetSpecularComponent
		{
			static float thunk(const RE::TESLandTexture* a_LT)
			{
				const auto manager = SeasonManager::GetSingleton();

				const auto swapLT = manager->CanSwapLandscape() ? manager->GetSwapLandTexture(a_LT) : nullptr;
				return swapLT ? swapLT->specularExponent : a_LT->specularExponent;
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static void Install()
			{
				constexpr std::array<std::uint64_t, 2> offsets{
				    OFFSET(0x160,0x162),
				    0x1D4
				};
				for (const auto& offset : offsets) {
					stl::write_thunk_call<GetSpecularComponent>(create_land_geometry.address() + offset);
				}
			}
		};

		struct GetAsShaderTextureSet
		{
			static RE::BSTextureSet* thunk(RE::BGSTextureSet* a_txst)
			{
				const auto manager = SeasonManager::GetSingleton();

				const auto swapLT = manager->CanSwapLandscape() ? manager->GetSwapLandTexture(a_txst) : nullptr;
				return swapLT ? swapLT->textureSet : a_txst;
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static void Install()
			{
				constexpr std::array<std::uint64_t, 3> offsets{
				    OFFSET(0x172,0x174),
					OFFSET(0x18B, 0x18D),
				    0x1E6
				};
				for (const auto& offset : offsets) {
					stl::write_thunk_call<GetAsShaderTextureSet>(create_land_geometry.address() + offset);
				}
			}
		};

		inline void Install()
		{
			IsConsideredSnow::Install();
			GetSpecularComponent::Install();
			GetAsShaderTextureSet::Install();
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
					if (const auto seasonManager = SeasonManager::GetSingleton(); seasonManager->CanSwapGrass(false)) {
						const auto swapLandTexture = seasonManager->GetSwapLandTexture(a_landTexture);

						return swapLandTexture ? swapLandTexture->textureGrassList : a_landTexture->textureGrassList;
					}
					return a_landTexture->textureGrassList;
				}

				static inline size_t size = 0x5;
				static inline auto id = RELOCATION_ID(18414, 18845);
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

					const auto grass = seasonManager->CanSwapGrass(true) ? RE::TESForm::LookupByID(a_param->grassFormID) : nullptr;
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

					std::make_pair(RELOCATION_ID(15204, 15372), 0x2F5),                 // add_cell_grass
					std::make_pair(RELOCATION_ID(15205, 15373), OFFSET(0x62B, 0x597)),  // add_cell_grass_from_buffer
					std::make_pair(RELOCATION_ID(15206, 15374), 0x25C),                 // add_cell_grass_from_file
				};

				for (const auto& [id, offset] : targets) {
					REL::Relocation<std::uintptr_t> target{ id, offset };
					stl::write_thunk_call<LoadGrassType>(target.address());
				}
			}
		}

		inline void Install()
		{
			Alt::Install();
			Standard::Install();
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
			static inline auto id = RELOCATION_ID(18418, 18849);
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
