#include "Seasons.h"

void Season::CheckLODExists()
{
	const auto& [seasonType, suffix] = ID;

	REX::INFO("{}", seasonType);

	//make sure LOD has been generated! No need to check form swaps
	const auto check_if_lod_exists = [&](bool& a_swaplod, std::string_view a_lodType, std::string_view a_folderPath, std::string_view a_fileType) {
		if (a_swaplod) {
			bool exists = false;
			bool existsInBSA = false;
			if (std::filesystem::exists(a_folderPath)) {
				for (const auto& entry : std::filesystem::directory_iterator(a_folderPath)) {
					if (entry.is_regular_file() && entry.path().string().contains(suffix)) {
						exists = true;
						break;
					}
				}
			}
			if (!exists) {
				std::string filePath = std::format(R"({}\Tamriel.4.0.0.{}.{})", a_folderPath, suffix, a_fileType);
				filePath.erase(0, 5);  // remove "Data/"
				const RE::BSResourceNiBinaryStream fileStream(filePath);
				if (fileStream.good()) {
					existsInBSA = true;
				}
			}
			if (!exists && !existsInBSA) {
				a_swaplod = false;
				REX::WARN("\t{} LOD files not found! Fallback to default LOD", a_lodType);
			} else {
				REX::INFO("\t{} LOD files found ({})", a_lodType, existsInBSA ? "BSA" : "Loose");
			}
		}
	};

	check_if_lod_exists(swapTerrainLOD, "Terrain", R"(Data\Meshes\Terrain\Tamriel)", "btr");
	check_if_lod_exists(swapObjectLOD, "Object", R"(Data\Meshes\Terrain\Tamriel\Objects)", "bto");
	check_if_lod_exists(swapTreeLOD, "Tree", R"(Data\Meshes\Terrain\Tamriel\Trees)", "btt");
}

bool Season::CanApplySnowShader() const
{
	return season == SEASON_TYPE::kWinter && is_in_valid_worldspace();
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

SEASON_TYPE Season::GetType() const
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

	auto& vec = stl::get_setting_ref(validWorldspaces);

	if (!values.empty()) {
		std::ranges::transform(values, std::back_inserter(vec), [&](const auto& val) { return val.pItem; });
	}
}

void Season::SaveData(CSimpleIniA& a_ini)
{
	auto& worldspaces = stl::get_setting_ref(validWorldspaces);

	std::ranges::sort(worldspaces);
	worldspaces.erase(std::ranges::unique(worldspaces).begin(), worldspaces.end());

	INI::set_value(a_ini, worldspaces, ID.type.data(), "Worldspaces", ";Valid worldspaces");
}

void Season::LoadWorldspaces()
{
	for (const auto& worldspace : stl::get_setting_ref(validWorldspaces)) {
		if (auto worldspacePtr = RE::TESForm::LookupByEditorID<RE::TESWorldSpace>(worldspace)) {
			validWorldspacesPtr.emplace(worldspacePtr);
		}
	}
}
