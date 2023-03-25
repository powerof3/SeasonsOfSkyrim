#pragma once

#include "FormSwapMap.h"

enum class SEASON : std::uint32_t
{
	kNone = 0,
	kWinter,
	kSpring,
	kSummer,
	kAutumn
};

enum class SEASON_TYPE : std::uint32_t
{
	kOff = 0,
	kPermanentWinter,
	kPermanentSpring,
	kPermanentSummer,
	kPermanentAutumn,
	kSeasonal
};

//type, suffix (Winter, WIN)
struct SEASON_ID
{
	std::string type{};
	std::string suffix{};
};

enum class LOD_TYPE : std::uint32_t
{
	kTerrain = 0,
	kObject,
	kTree
};

class Season
{
public:
	explicit Season(SEASON a_season, SEASON_ID a_ID) :
		season(a_season),
		ID(std::move(a_ID))
	{}

	void LoadSettings(CSimpleIniA& a_ini, bool a_writeComment = false);
	void CheckLODExists();

	[[nodiscard]] bool CanApplySnowShader() const;
	[[nodiscard]] bool CanSwapForm(RE::FormType a_formType) const;
	[[nodiscard]] bool CanSwapLOD(LOD_TYPE a_type) const;
	[[nodiscard]] bool CanSwapLandscape() const;

	[[nodiscard]] const SEASON_ID& GetID() const;
	[[nodiscard]] SEASON           GetType() const;

	[[nodiscard]] FormSwapMap& GetFormSwapMap();
	void                       LoadData(const CSimpleIniA& a_ini);
	void                       SaveData(CSimpleIniA& a_ini);

private:
	[[nodiscard]] bool is_valid_swap_type(const RE::FormType a_formType) const
	{
		switch (a_formType) {
		case RE::FormType::Activator:
			return swapActivators;
		case RE::FormType::Furniture:
			return swapFurniture;
		case RE::FormType::MovableStatic:
			return swapMovableStatics;
		case RE::FormType::Static:
			return swapStatics;
		case RE::FormType::Tree:
			return swapTrees;
		case RE::FormType::Grass:
			return swapGrass;
		case RE::FormType::Flora:
			return swapFlora;
		case RE::FormType::ReferenceEffect:
			return swapVFX;
		default:
			return false;
		}
	}
	[[nodiscard]] bool is_in_valid_worldspace() const
	{
		const auto worldSpace = RE::TES::GetSingleton()->worldSpace;
		return worldSpace && std::ranges::find(validWorldspaces, worldSpace->GetFormEditorID()) != validWorldspaces.end();
	}

	SEASON    season{};
	SEASON_ID ID{};

	std::vector<std::string> validWorldspaces{
		"Tamriel",
		"MarkarthWorld",
		"RiftenWorld",
		"SolitudeWorld",
		"WhiterunWorld",
		"DLC1HunterHQWorld",
		"DLC2SolstheimWorld"
	};

	bool swapActivators{ true };
	bool swapFurniture{ true };
	bool swapMovableStatics{ true };
	bool swapStatics{ true };
	bool swapTrees{ true };
	bool swapFlora{ true };
	bool swapVFX{ true };

	bool swapObjectLOD{ true };
	bool swapTerrainLOD{ true };
	bool swapTreeLOD{ true };

	bool swapGrass{ true };

	FormSwapMap formMap{};
};
