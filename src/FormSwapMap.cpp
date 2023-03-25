#include "FormSwapMap.h"

FormSwapMap::FormSwapMap()
{
	for (auto& type : recordTypes) {
		_formMap.emplace(type, MapPair<RE::FormID>{});
	}
}

RE::TESLandTexture* FormSwapMap::GenerateLandTextureSnowVariant(const RE::TESLandTexture* a_landTexture)
{
	static constexpr std::array blackList = { "Snow"sv, "Ice"sv, "Winter"sv, "Frozen"sv, "Coast"sv, "River"sv };

	const auto editorID = util::get_editorID(a_landTexture);
	if (!editorID.empty() && std::ranges::any_of(blackList, [&](const auto str) { return string::icontains(editorID, str); })) {
		return nullptr;
	}

	static constexpr RE::FormID LSnow01 = 0x0000089B;
	static constexpr RE::FormID LSnow02 = 0x0006A1B1;

	const auto mat = a_landTexture->materialType;
	const auto matID = mat ? mat->materialID : RE::MATERIAL_ID::kNone;

	RE::FormID formID;

	switch (matID) {
	case RE::MATERIAL_ID::kGrass:
		{
			static constexpr RE::FormID LGrassSnow01NoGrass = 0x0008B01E;
			static constexpr RE::FormID LGrassSnow01 = 0x00000894;

			switch (a_landTexture->GetFormID()) {
			case 0x0001342A:  // LFieldGrass02
			case 0x00024E46:  // LFieldGrass01NoGrass
			case 0x00024E30:  // LTundra01
			case 0x000A2741:  // LTundra01NoGrass
				formID = LSnow01;
				break;
			case 0x000134B7:  // LFieldDirtGrass01
			case 0x000300E4:  // LTundra02
				formID = LSnow02;
				break;
			default:
				formID = !a_landTexture->textureGrassList.empty() ? LGrassSnow01 : LGrassSnow01NoGrass;
				break;
			}
		}
		break;
	case RE::MATERIAL_ID::kDirt:
		{
			static constexpr RE::FormID LDirtSnowPath01 = 0x0001B082;

			switch (a_landTexture->GetFormID()) {
			case 0x00000C16:  // LDirt02
				formID = LSnow02;
				break;
			case 0xB424C:  // LDirtPath01
				formID = LDirtSnowPath01;
				break;
			default:
				formID = LSnow01;
				break;
			}
		}
		break;
	case RE::MATERIAL_ID::kStone:
	case RE::MATERIAL_ID::kStoneBroken:
	case RE::MATERIAL_ID::kGravel:
		{
			static constexpr RE::FormID LSnowRocks01 = 0x0006A1AF;
			static constexpr RE::FormID LSnowRockswGrass = 0x000F871F;

			switch (a_landTexture->GetFormID()) {
			case 0x0002C6C6:  // LTundraRocks01
				formID = LSnowRocks01;
				break;
			case 0x0006DE8B:  // LTundraRocks01NoRocks
				formID = LSnow01;
				break;
			default:
				formID = !a_landTexture->textureGrassList.empty() ? LSnowRockswGrass : LSnowRocks01;
				break;
			}
		}
		break;
	case RE::MATERIAL_ID::kSnow:
	case RE::MATERIAL_ID::kIce:
	case RE::MATERIAL_ID::kSand:
	case RE::MATERIAL_ID::kMud:
		return nullptr;
	default:
		formID = LSnow02;
		break;
	}

	return RE::TESForm::LookupByID<RE::TESLandTexture>(formID);
}

void FormSwapMap::LoadFormSwaps(const std::string& a_type, const std::vector<std::string>& a_values)
{
	auto& map = get_map(a_type);
	for (const auto& key : a_values) {
		const auto formPair = string::split(key, "|");

		const auto formID = INI::parse_form(formPair[kBase]);
		const auto swapFormID = INI::parse_form(formPair[kSwap]);

		if (formID != 0 && swapFormID != 0) {
			map.insert_or_assign(formID, swapFormID);
		}
	}
}

void FormSwapMap::LoadFormSwaps(const CSimpleIniA& a_ini)
{
	for (auto& type : recordTypes) {
		CSimpleIniA::TNamesDepend values;
		a_ini.GetAllKeys(type.c_str(), values);
		values.sort(CSimpleIniA::Entry::LoadOrder());

		if (!values.empty()) {
			logger::info("\t\t[{}] read {} variants", type, values.size());

			std::vector<std::string> vec;
			std::ranges::transform(values, std::back_inserter(vec), [&](const auto& val) { return val.pItem; });

			LoadFormSwaps(type, vec);
		}
	}
}

//only covers winter
bool FormSwapMap::GenerateFormSwaps(CSimpleIniA& a_ini, bool a_forceRegenerate)
{
	bool save = false;

	for (auto& type : standardTypes) {
		CSimpleIniA::TNamesDepend values;
		a_ini.GetAllKeys(type.c_str(), values);
		values.sort(CSimpleIniA::Entry::LoadOrder());

		if (values.empty() || a_forceRegenerate) {
			save = true;

			if (a_forceRegenerate) {
				a_ini.Delete(type.c_str(), nullptr, true);
			}

			switch (string::const_hash(type)) {
			case string::const_hash("LandTextures"sv):
				{
					TempFormSwapMap<RE::TESLandTexture> landTxstMap;
					get_snow_variants(a_ini, type, landTxstMap);
				}
				break;
			case string::const_hash("Activators"sv):
				{
					TempFormSwapMap<RE::TESObjectACTI> activatorMap;
					get_snow_variants(a_ini, type, activatorMap);
				}
				break;
			case string::const_hash("Furniture"sv):
				{
					TempFormSwapMap<RE::TESFurniture> furnitureMap;
					get_snow_variants(a_ini, type, furnitureMap);
				}
				break;
			case string::const_hash("MovableStatics"sv):
				{
					TempFormSwapMap<RE::BGSMovableStatic> movStaticMap;
					get_snow_variants(a_ini, type, movStaticMap);
				}
				break;
			case string::const_hash("Statics"sv):
				{
					TempFormSwapMap<RE::TESObjectSTAT> staticMap;
					get_snow_variants(a_ini, type, staticMap);
				}
				break;
			case string::const_hash("Trees"sv):
				{
					TempFormSwapMap<RE::TESObjectTREE> treeMap;
					get_snow_variants(a_ini, type, treeMap);
				}
				break;
			default:
				break;
			}
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

RE::TESLandTexture* FormSwapMap::GetSwapLandTexture(const RE::TESLandTexture* a_landTxst)
{
	const auto& map = _formMap["LandTextures"];
	if (map.empty()) {
		return nullptr;
	}

	const auto it = map.find(a_landTxst->GetFormID());
	return it != map.end() ? RE::TESForm::LookupByID<RE::TESLandTexture>(it->second) : nullptr;
}

RE::TESLandTexture* FormSwapMap::GetSwapLandTexture(const RE::BGSTextureSet* a_txst)
{
	const auto landTexture = Cache::DataHolder::GetSingleton()->GetLandTextureFromTextureSet(a_txst);
	return GetSwapLandTexture(landTexture);
}
