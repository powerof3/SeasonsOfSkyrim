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
                if (const auto seasonManager = SeasonManager::GetSingleton(); seasonManager->IsLandscapeSwapAllowed()) {
					const auto newLandTexture = seasonManager->GetLandTextureFromTextureSet(a_textureSet);
					return newLandTexture ? newLandTexture->textureSet : a_textureSet;
				}
				return a_textureSet;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	namespace Grass
	{
		struct GetGrassList
		{
			static RE::BSSimpleList<RE::TESGrass*>& func(RE::TESLandTexture* a_landTexture)
			{
				if (const auto seasonManager = SeasonManager::GetSingleton(); seasonManager->CanSwapGrass()) {
					const auto newLandTexture = seasonManager->GetLandTexture(a_landTexture);
					return newLandTexture ? newLandTexture->textureGrassList : a_landTexture->textureGrassList;
				}
				return a_landTexture->textureGrassList;
			}

			static inline size_t size = 0x5;
			static inline std::uint64_t id = 18414;
		};
	}

	namespace Material
	{
		struct GetHavokMaterialType
		{
			static RE::MATERIAL_ID func(const RE::TESLandTexture* a_landTexture)
			{
				if (const auto seasonManager = SeasonManager::GetSingleton(); seasonManager->IsLandscapeSwapAllowed()) {
					const auto newLandTexture = seasonManager->GetLandTexture(a_landTexture);
					const auto materialType = newLandTexture ? newLandTexture->materialType : a_landTexture->materialType;

					return materialType ? materialType->materialID : RE::MATERIAL_ID::kNone;
				}
				return a_landTexture->materialType ? a_landTexture->materialType->materialID : RE::MATERIAL_ID::kNone;
			}

			static inline size_t size = 0xE;
			static inline std::uint64_t id = 18418;
		};
	}

	inline void Install()
	{
		REL::Relocation<std::uintptr_t> create_land_geometry{ REL::ID(18368) };
		stl::write_thunk_call<Texture::GetAsShaderTextureSet>(create_land_geometry.address() + 0x172);
		stl::write_thunk_call<Texture::GetAsShaderTextureSet>(create_land_geometry.address() + 0x18B);
		stl::write_thunk_call<Texture::GetAsShaderTextureSet>(create_land_geometry.address() + 0x1E6);

		stl::asm_replace<Grass::GetGrassList>();
		stl::asm_replace<Material::GetHavokMaterialType>();
	}
}
