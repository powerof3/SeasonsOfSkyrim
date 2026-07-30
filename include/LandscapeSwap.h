#pragma once

#include "SeasonManager.h"

namespace LandscapeSwap
{
	namespace Texture
	{
		static inline REL::Relocation<std::uintptr_t> create_land_geometry{ RELOCATION_ID(18368, 18791) };

		namespace IsConsideredSnow
		{
			template <std::size_t N>
			struct Func
			{
				static float thunk(const RE::TESLandTexture* a_LT)
				{
					const auto swapLT = SeasonManager::GetSingleton()->GetSwapLandTexture(a_LT);
					return swapLT ? swapLT->shaderTextureIndex != 0 : a_LT->shaderTextureIndex != 0;
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install()
			{
				constexpr std::array<std::uint64_t, 2> offsets{
					OFFSET(0x155, 0x157),
					0x1C9
				};

				stl::write_thunk_call<Func<0>>(create_land_geometry.address() + offsets[0]);
				stl::write_thunk_call<Func<1>>(create_land_geometry.address() + offsets[1]);
			}
		}

		namespace GetSpecularComponent
		{
			template <std::size_t N>
			struct Func
			{
				static float thunk(const RE::TESLandTexture* a_LT)
				{
					const auto swapLT = SeasonManager::GetSingleton()->GetSwapLandTexture(a_LT);
					return swapLT ? swapLT->specularExponent : a_LT->specularExponent;
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install()
			{
				constexpr std::array<std::uint64_t, 2> offsets{
					OFFSET(0x160, 0x162),
					0x1D4
				};

				stl::write_thunk_call<Func<0>>(create_land_geometry.address() + offsets[0]);
				stl::write_thunk_call<Func<1>>(create_land_geometry.address() + offsets[1]);
			}
		}

		namespace GetAsShaderTextureSet
		{

			template <std::size_t N>
			struct Func
			{
				static RE::BSTextureSet* thunk(RE::BGSTextureSet* a_txst)
				{
					const auto swapLT = SeasonManager::GetSingleton()->GetSwapLandTexture(a_txst);
					if (swapLT == nullptr) {
						// no swap found
						if (a_txst != nullptr) {
							a_txst->pad12C = 0;  // Reset pad12C if no swap is found
						}
						return a_txst;
					}

					const auto swapTXST = swapLT->textureSet;
					if (a_txst != nullptr && swapTXST != nullptr) {
						a_txst->pad12C = swapTXST->formID;  // Set pad12C to swapped TXST formid
					}
					return swapTXST;
				}
				static inline REL::Relocation<decltype(thunk)> func;
			};

			inline void Install()
			{
				constexpr std::array<std::uint64_t, 3> offsets{
					OFFSET(0x172, 0x174),
					OFFSET(0x18B, 0x18D),
					0x1E6
				};

				stl::write_thunk_call<Func<0>>(create_land_geometry.address() + offsets[0]);
				stl::write_thunk_call<Func<1>>(create_land_geometry.address() + offsets[1]);
				stl::write_thunk_call<Func<2>>(create_land_geometry.address() + offsets[2]);
			}
		}

		inline void Install()
		{
			IsConsideredSnow::Install();
			GetSpecularComponent::Install();
			GetAsShaderTextureSet::Install();
		}
	}

	namespace Grass
	{
		struct GetGrassList
		{
			static RE::BSSimpleList<RE::TESGrass*>& func(RE::TESLandTexture* a_landTexture)
			{
				const auto swapLandTexture = SeasonManager::GetSingleton()->GetSwapLandTextureForGrass(a_landTexture);
				return swapLandTexture ? swapLandTexture->textureGrassList : a_landTexture->textureGrassList;
			}

			static inline std::size_t size = 0x5;
			static inline auto        id = RELOCATION_ID(18414, 18845);
		};

		inline void Install()
		{
			stl::asm_replace<GetGrassList>();
		}
	}

	namespace Material
	{
		struct GetHavokMaterialType
		{
			static RE::MATERIAL_ID func(const RE::TESLandTexture* a_landTexture)
			{
				const auto swapLandTexture = SeasonManager::GetSingleton()->GetSwapLandTexture(a_landTexture);
				const auto materialType = swapLandTexture ? swapLandTexture->materialType : a_landTexture->materialType;

				return materialType ? materialType->materialID : RE::MATERIAL_ID::kNone;
			}

			static inline std::size_t size = 0xE;
			static inline auto        id = RELOCATION_ID(18418, 18849);
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
