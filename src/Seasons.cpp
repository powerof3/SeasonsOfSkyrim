#include "Seasons.h"

FormSwapMap::FormSwapMap()
{
	for (auto& type : formTypes) {
		_formMap.emplace(type, Map::FormID{});
	}
}

void FormSwapMap::LoadFormSwaps_Impl(const std::string& a_type, const std::vector<std::string>& a_values)
{
	auto& map = get_map(a_type);
	for (const auto& key : a_values) {
		const auto formPair = string::split(key, "|");

		constexpr auto get_form = [](const std::string& a_str) {
			if (a_str.find('~') != std::string::npos) {
				const auto formPair = string::split(a_str, "~");

				const auto processedFormPair = std::make_pair(
					string::lexical_cast<RE::FormID>(formPair[0], true), formPair[1]);

				return RE::TESDataHandler::GetSingleton()->LookupFormID(processedFormPair.first, processedFormPair.second);
			}
			if (const auto form = RE::TESForm::LookupByEditorID(a_str); form) {
				return form->GetFormID();
			}
			return static_cast<RE::FormID>(0);
		};

		auto formID = get_form(formPair[kBase]);
		auto swapFormID = get_form(formPair[kSwap]);

		if (swapFormID != 0 && formID != 0) {
			map.insert_or_assign(formID, swapFormID);
		}
	}
}

RE::TESLandTexture* FormSwapMap::GenerateLandTextureSnowVariant(const RE::TESLandTexture* a_landTexture)
{
	static std::array blackList = { "Snow"sv, "Ice"sv, "Winter"sv, "Frozen"sv, "Coast"sv, "River"sv };

	if (const auto editorID = util::get_editorID(a_landTexture); !editorID.empty() && std::ranges::any_of(blackList, [&](const auto str) { return editorID.find(str) != std::string::npos; })) {
		return nullptr;
	}

	const auto mat = a_landTexture->materialType;
	const RE::MATERIAL_ID matID = mat ? mat->materialID : RE::MATERIAL_ID::kNone;

	RE::FormID formID;

	switch (matID) {
	case RE::MATERIAL_ID::kGrass:
		{
			switch (a_landTexture->GetFormID()) {
			case 0x57DC7:             //LFallForestLeaves01
				formID = 0x02003ACE;  //LWinterForestLeaves01
				break;
			default:
				formID = !a_landTexture->textureGrassList.empty() ? 0x00000894 : 0x0008B01E;  //LGrassSnow01 : LGrassSnow01NoGrass
				break;
			}
		}
		break;
	case RE::MATERIAL_ID::kDirt:
		{
			switch (a_landTexture->GetFormID()) {
			case 0xB424C:             //LDirtPath01
				formID = 0x0001B082;  //LDirtSnowPath01
				break;
			case 0x57DCF:             //LFallForestDirt01
				formID = 0x02005233;  //LWinterForestDirt01
				break;
			default:
				formID = 0x0000089B;  //LSnow01
				break;
			}
		}
		break;
	case RE::MATERIAL_ID::kStone:
	case RE::MATERIAL_ID::kStoneBroken:
	case RE::MATERIAL_ID::kGravel:
		formID = !a_landTexture->textureGrassList.empty() ? 0x000F871F : 0x0006A1AF;  //LSnowRockswGrass : LSnowRocks01
		break;
	case RE::MATERIAL_ID::kSnow:
	case RE::MATERIAL_ID::kIce:
		return nullptr;
	default:
		formID = 0x0006A1B1;  //LSnow2
		break;
	}

	return RE::TESForm::LookupByID<RE::TESLandTexture>(formID);
}

void FormSwapMap::LoadFormSwaps(const CSimpleIniA& a_ini)
{
	for (auto& type : formTypes) {
		if (const auto values = a_ini.GetSection(type.c_str()); values && !values->empty()) {
			logger::info("	[{}] read {} variants", type, values ? values->size() : -1);

			std::vector<std::string> vec;
			std::ranges::transform(*values, std::back_inserter(vec), [&](const auto& val) { return val.first.pItem; });

			LoadFormSwaps_Impl(type, vec);
		}
	}
}

//only covers winter
bool FormSwapMap::GenerateFormSwaps(CSimpleIniA& a_ini)
{
	bool save = false;

	for (auto& type : formTypes) {
		if (const auto values = a_ini.GetSection(type.c_str()); !values || values->empty()) {
			save = true;

			if (type == "Statics") {
				TempFormSwapMap<RE::TESObjectSTAT> staticMap;
				get_snow_variants(a_ini, type, staticMap);
			} else if (type == "Activators") {
				TempFormSwapMap<RE::TESObjectACTI> activatorMap;
				get_snow_variants(a_ini, type, activatorMap);
			} else if (type == "Furniture") {
				TempFormSwapMap<RE::TESFurniture> activatorMap;
				get_snow_variants(a_ini, type, activatorMap);
			} else if (type == "LandTextures") {
				TempFormSwapMap<RE::TESLandTexture> landTxstMap;
				get_snow_variants(a_ini, type, landTxstMap);
			} else if (type == "MovableStatics") {
				TempFormSwapMap<RE::BGSMovableStatic> movStaticMap;
				get_snow_variants(a_ini, type, movStaticMap);
			} else if (type == "Trees") {
				TempFormSwapMap<RE::TESObjectTREE> treeMap;
				get_snow_variants(a_ini, type, treeMap);
			}
		} else {
			logger::info("	[{}] read {} variants", type, values ? values->size() : -1);

			std::vector<std::string> vec;
			std::ranges::transform(*values, std::back_inserter(vec), [&](const auto& val) { return val.first.pItem; });

			LoadFormSwaps_Impl(type, vec);
		}
	}

	return save;
}

RE::TESBoundObject* FormSwapMap::GetSwapForm(const RE::TESForm* a_form)
{
	auto& map = get_map(a_form->GetFormType());
	if (map.empty()) {
		return nullptr;
	}

	const auto it = map.find(a_form->GetFormID());
	return it != map.end() ? RE::TESForm::LookupByID<RE::TESBoundObject>(it->second) : nullptr;
}

RE::TESLandTexture* FormSwapMap::GetSwapLandTexture(const RE::TESForm* a_form)
{
	const auto& map = _formMap["LandTextures"];
	if (map.empty()) {
		return nullptr;
	}

	const auto it = map.find(a_form->GetFormID());
	return it != map.end() ? RE::TESForm::LookupByID<RE::TESLandTexture>(it->second) : nullptr;
}

RE::TESLandTexture* FormSwapMap::GetSwapLandTextureFromTextureSet(const RE::BGSTextureSet* a_txst)
{
	const auto landTexture = Cache::DataHolder::GetSingleton()->GetLandTextureFromTextureSet(a_txst);
	return GetSwapLandTexture(landTexture);
}

void Season::LoadSettingsAndVerify(CSimpleIniA& a_ini)
{
	const auto& [type, suffix] = ID;

	INI::get_value(a_ini, allowedWorldspaces, type.c_str(), "Worldspaces", ";Valid worldspaces");
	INI::get_value(a_ini, swapActivators, type.c_str(), "Activators", ";Swap objects of these types for seasonal variants");
	INI::get_value(a_ini, swapFurniture, type.c_str(), "Furniture", nullptr);
	INI::get_value(a_ini, swapMovableStatics, type.c_str(), "Movable Statics", nullptr);
	INI::get_value(a_ini, swapStatics, type.c_str(), "Trees", nullptr);
	INI::get_value(a_ini, swapLOD, type.c_str(), "LOD", ";Seasonal LOD must be generated using DynDOLOD Alpha 65/SSELODGen Beta 86 or higher.\n;See https://dyndolod.info/Help/Seasons for more info");
	INI::get_value(a_ini, swapGrass, type.c_str(), "Grass", ";Enable seasonal grass types (eg. snow grass in winter)");

	if (swapLOD) {  //make sure LOD has been generated! No need to check form swaps
		const auto worldSpaceName = !allowedWorldspaces.empty() ? allowedWorldspaces[0] : "Tamriel";
		const auto lodPath = fmt::format(R"(Data\meshes\terrain\{}\{}.4.0.0.{}.BTR)", worldSpaceName, worldSpaceName, suffix);

		swapLOD = std::filesystem::exists(lodPath);
		if (!swapLOD) {
			logger::error("LOD files for season {} not found! Default LOD will be used instead", type);
		}
	}
}

bool Season::CanSwapGrass() const
{
	return swapGrass;
}

bool Season::CanSwapLOD() const
{
	return swapLOD;
}

bool Season::IsLandscapeSwapAllowed() const
{
	return is_in_valid_worldspace();
}

bool Season::IsSwapAllowed(const RE::TESForm* a_form) const
{
	return is_valid_swap_type(a_form) && is_in_valid_worldspace();
}

const std::pair<std::string, std::string>& Season::GetID() const
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
