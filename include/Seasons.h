#pragma once

#include "FormSwapMap.h"

enum class SEASON_TYPE : std::uint32_t
{
	kNone = 0,
	kWinter,
	kSpring,
	kSummer,
	kAutumn,

	kTotal
};

enum class SEASON_MODE : std::uint32_t
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
	std::string_view type{};
	std::string_view suffix{};
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
	explicit Season(SEASON_TYPE a_season, SEASON_ID a_ID, bool a_writeComment) :
		ID(std::move(a_ID)),
		season(a_season),
		validWorldspaces(ID.type, "Worldspaces", a_writeComment ? ";Valid worldspaces."sv : "", "Tamriel|MarkarthWorld|RiftenWorld|SolitudeWorld|WhiterunWorld|DLC1HunterHQWorld|DLC2SolstheimWorld"s),
		swapActivators(ID.type, "Activators", a_writeComment ? ";Swap objects of these types for seasonal variants."sv : "", true),
		swapFurniture(ID.type, "Furniture", "", true),
		swapMovableStatics(ID.type, "Movable Statics", "", true),
		swapStatics(ID.type, "Statics", "", true),
		swapTrees(ID.type, "Trees", "", true),
		swapFlora(ID.type, "Flora", "", true),
		swapVFX(ID.type, "Visual Effects", "", true),
		swapObjectLOD(ID.type, "Object LOD", a_writeComment ? ";Seasonal LOD must be generated using DynDOLOD Alpha 67/SSELODGen Beta 88 or higher.\n"
															  ";See https://dyndolod.info/Help/Seasons for more info"sv :
															  "",
			true),
		swapTerrainLOD(ID.type, "Terrain LOD", "", true),
		swapTreeLOD(ID.type, "Tree LOD", "", true),
		swapGrass(ID.type, "Grass", a_writeComment ? ";Enable seasonal grass types (eg. snow grass in winter)."sv : "", true)
	{}

	void CheckLODExists();

	[[nodiscard]] bool CanApplySnowShader() const;
	[[nodiscard]] bool CanSwapForm(RE::FormType a_formType) const;
	[[nodiscard]] bool CanSwapLOD(LOD_TYPE a_type) const;
	[[nodiscard]] bool CanSwapLandscape() const;

	[[nodiscard]] const SEASON_ID& GetID() const;
	[[nodiscard]] SEASON_TYPE      GetType() const;

	[[nodiscard]] FormSwapMap& GetFormSwapMap();
	void                       LoadData(const CSimpleIniA& a_ini);
	void                       SaveData(CSimpleIniA& a_ini);
	void                       LoadWorldspaces();

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
		return worldSpace && validWorldspacesPtr.contains(worldSpace);
	}

	// members
	SEASON_ID   ID{};
	SEASON_TYPE season{};

	Setting::StrA               validWorldspaces;
	FlatSet<RE::TESWorldSpace*> validWorldspacesPtr{};

	Setting::Bool swapActivators;
	Setting::Bool swapFurniture;
	Setting::Bool swapMovableStatics;
	Setting::Bool swapStatics;
	Setting::Bool swapTrees;
	Setting::Bool swapFlora;
	Setting::Bool swapVFX;

	Setting::Bool swapObjectLOD;
	Setting::Bool swapTerrainLOD;
	Setting::Bool swapTreeLOD;

	Setting::Bool swapGrass;

	FormSwapMap formMap{};
};
