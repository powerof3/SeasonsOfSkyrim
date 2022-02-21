#include "FormSwapMap.h"

FormSwapMap::FormSwapMap()
{
	for (auto& type : recordTypes) {
		_formMap.emplace(type, MapPair<RE::FormID>{});
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
		formID = !a_landTexture->textureGrassList.empty() ? 0x00000894 : 0x0008B01E;  //LGrassSnow01 : LGrassSnow01NoGrass
		break;
	case RE::MATERIAL_ID::kDirt:
		{
			if (a_landTexture->GetFormID() == 0xB424C) {  //LDirtPath01
				formID = 0x0001B082;                      //LDirtSnowPath01
			} else {
				formID = 0x0000089B;  //LSnow01
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

void FormSwapMap::LoadFormSwaps_Impl(const std::string& a_type, const std::vector<std::string>& a_values)
{
	auto& map = get_map(a_type);
	for (const auto& key : a_values) {
		const auto formPair = string::split(key, "|");

		auto formID = INI::parse_form(formPair[kBase]);
		auto swapFormID = INI::parse_form(formPair[kSwap]);

		if (swapFormID != 0 && formID != 0) {
			map.insert_or_assign(formID, swapFormID);
		}
	}
}

void FormSwapMap::LoadFormSwaps(const CSimpleIniA& a_ini)
{
	for (auto& type : recordTypes) {
		if (const auto values = a_ini.GetSection(type.c_str()); values && !values->empty()) {
			logger::info("		[{}] read {} variants", type, values ? values->size() : -1);

			std::vector<std::string> vec;
			std::ranges::transform(*values, std::back_inserter(vec), [&](const auto& val) { return val.first.pItem; });

			LoadFormSwaps_Impl(type, vec);
		}
	}
}

//only covers winter
bool FormSwapMap::GenerateFormSwaps(CSimpleIniA& a_ini, bool a_forceRegenerate)
{
	bool save = false;

	for (auto& type : recordTypes) {
		if (const auto values = a_ini.GetSection(type.c_str()); !values || values->empty() || a_forceRegenerate) {
			save = true;

			if (type == "LandTextures") {
				TempFormSwapMap<RE::TESLandTexture> landTxstMap;
				get_snow_variants(a_ini, type, landTxstMap);
			} else if (type == "Activators") {
				TempFormSwapMap<RE::TESObjectACTI> activatorMap;
				get_snow_variants(a_ini, type, activatorMap);
			} else if (type == "Furniture") {
				TempFormSwapMap<RE::TESFurniture> activatorMap;
				get_snow_variants(a_ini, type, activatorMap);
			} else if (type == "MovableStatics") {
				TempFormSwapMap<RE::BGSMovableStatic> movStaticMap;
				get_snow_variants(a_ini, type, movStaticMap);
			} else if (type == "Statics") {
				TempFormSwapMap<RE::TESObjectSTAT> staticMap;
				get_snow_variants(a_ini, type, staticMap);
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
