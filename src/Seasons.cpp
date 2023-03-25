#include "Seasons.h"

void Season::LoadSettings(CSimpleIniA& a_ini, bool a_writeComment)
{
	const auto& [seasonType, suffix] = ID;

	ini::get_value(a_ini, validWorldspaces, seasonType.c_str(), "Worldspaces", a_writeComment ? ";Valid worldspaces." : ";");

	ini::get_value(a_ini, swapActivators, seasonType.c_str(), "Activators", a_writeComment ? ";Swap objects of these types for seasonal variants." : ";");
	ini::get_value(a_ini, swapFurniture, seasonType.c_str(), "Furniture", nullptr);
	ini::get_value(a_ini, swapMovableStatics, seasonType.c_str(), "Movable Statics", nullptr);
	ini::get_value(a_ini, swapStatics, seasonType.c_str(), "Statics", nullptr);
	ini::get_value(a_ini, swapTrees, seasonType.c_str(), "Trees", nullptr);
	ini::get_value(a_ini, swapFlora, seasonType.c_str(), "Flora", nullptr);
	ini::get_value(a_ini, swapVFX, seasonType.c_str(), "Visual Effects", nullptr);

	ini::get_value(a_ini, swapObjectLOD, seasonType.c_str(), "Object LOD", a_writeComment ? ";Seasonal LOD must be generated using DynDOLOD Alpha 67/SSELODGen Beta 88 or higher.\n;See https://dyndolod.info/Help/Seasons for more info" : ";");
	ini::get_value(a_ini, swapTerrainLOD, seasonType.c_str(), "Terrain LOD", nullptr);
	ini::get_value(a_ini, swapTreeLOD, seasonType.c_str(), "Tree LOD", nullptr);

	ini::get_value(a_ini, swapGrass, seasonType.c_str(), "Grass", a_writeComment ? ";Enable seasonal grass types (eg. snow grass in winter)." : ";");
}

void Season::CheckLODExists()
{
	const auto& [seasonType, suffix] = ID;

	logger::info("{}", seasonType);

    //make sure LOD has been generated! No need to check form swaps
	const auto check_if_lod_exists = [&](bool& a_swaplod, std::string_view a_lodType, std::string_view a_folderPath, std::string_view a_fileType) {
		if (a_swaplod) {
			bool exists = false;
			bool existsInBSA = false;
			if (std::filesystem::exists(a_folderPath)) {
				for (const auto& entry : std::filesystem::directory_iterator(a_folderPath)) {
					if (entry.exists() && entry.is_regular_file() && entry.path().string().contains(suffix)) {
						exists = true;
						break;
					}
				}
			}
			if (!exists) {
				std::string filePath = fmt::format(R"({}\Tamriel.4.0.0.{}.{})", a_folderPath, suffix, a_fileType);
				filePath.erase(0, 5);  // remove "Data/"
			    const RE::BSResourceNiBinaryStream fileStream(filePath);
				if (fileStream.good()) {
					existsInBSA = true;
				}
			}
			if (!exists && !existsInBSA) {
				a_swaplod = false;
				logger::warn("\t{} LOD files not found! Fallback to default LOD", a_lodType);
			} else {
				logger::info("\t{} LOD files found ({})", a_lodType, existsInBSA ? "BSA" : "Loose");
			}
		}
	};

	check_if_lod_exists(swapTerrainLOD, "Terrain", R"(Data\Meshes\Terrain\Tamriel)", "btr");
	check_if_lod_exists(swapObjectLOD, "Object", R"(Data\Meshes\Terrain\Tamriel\Objects)", "bto");
	check_if_lod_exists(swapTreeLOD, "Tree", R"(Data\Meshes\Terrain\Tamriel\Trees)", "btt");
}

bool Season::CanApplySnowShader() const
{
	return season == SEASON::kWinter && is_in_valid_worldspace();
}

bool Season::CanSwapForm(RE::FormType a_formType) const
{
	return is_valid_swap_type(a_formType) && is_in_valid_worldspace();
}

bool Season::CanSwapLandscape() const
{
	return is_in_valid_worldspace();
}

bool Season::CanSwapLOD(const LOD_TYPE a_type) const
{
	if (!is_in_valid_worldspace()) {
		return false;
	}

	switch (a_type) {
	case LOD_TYPE::kTerrain:
		return swapTerrainLOD;
	case LOD_TYPE::kObject:
		return swapObjectLOD;
	case LOD_TYPE::kTree:
		return swapTreeLOD;
	default:
		return false;
	}
}

const SEASON_ID& Season::GetID() const
{
	return ID;
}

SEASON Season::GetType() const
{
	return season;
}

FormSwapMap& Season::GetFormSwapMap()
{
	return formMap;
}

void Season::LoadData(const CSimpleIniA& a_ini)
{
	formMap.LoadFormSwaps(a_ini);

	CSimpleIniA::TNamesDepend values;
	a_ini.GetAllKeys("Worldspaces", values);
	values.sort(CSimpleIniA::Entry::LoadOrder());

	if (!values.empty()) {
		std::ranges::transform(values, std::back_inserter(validWorldspaces), [&](const auto& val) { return val.pItem; });
	}
}

void Season::SaveData(CSimpleIniA& a_ini)
{
	std::ranges::sort(validWorldspaces);
	validWorldspaces.erase(std::ranges::unique(validWorldspaces).begin(), validWorldspaces.end());

	INI::set_value(a_ini, validWorldspaces, ID.type.c_str(), "Worldspaces", ";Valid worldspaces");
}
