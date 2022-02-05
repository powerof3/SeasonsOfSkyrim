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

enum class LOD_TYPE : std::uint32_t
{
	kTerrain = 0,
	kObject,
	kTree
};

class Season
{
public:
	explicit Season(SEASON a_season, std::pair<std::string, std::string> a_ID) :
		season(a_season),
		ID(std::move(a_ID))
	{}

	void LoadSettingsAndVerify(CSimpleIniA& a_ini, bool a_writeComment = false);

	[[nodiscard]] bool CanSwapGrass() const;

	[[nodiscard]] bool CanSwapLOD(LOD_TYPE a_type) const;
	
	[[nodiscard]] bool IsLandscapeSwapAllowed() const;
	[[nodiscard]] bool IsSwapAllowed(RE::FormType a_formType) const;

	[[nodiscard]] const std::pair<std::string, std::string>& GetID() const;
	[[nodiscard]] SEASON GetType() const;

	[[nodiscard]] FormSwapMap& GetFormSwapMap();

private:
	SEASON season{};
	std::pair<std::string, std::string> ID{};  //type, suffix (Winter, WIN)

	std::vector<std::string> allowedWorldspaces{
		"Tamriel",
		"MarkathWorld",
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

	bool swapObjectLOD{ true };
	bool swapTerrainLOD{ true };
	bool swapTreeLOD{ true };

	bool swapGrass{ true };

	FormSwapMap formMap{};

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
		default:
			return false;
		}
	}

    [[nodiscard]] bool is_in_valid_worldspace() const
	{
		const auto worldSpace = RE::TES::GetSingleton()->worldSpace;
		return worldSpace && std::ranges::find(allowedWorldspaces, worldSpace->GetFormEditorID()) != allowedWorldspaces.end();
	}
};
