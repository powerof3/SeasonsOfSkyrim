#include "FormSwapMap.h"

RE::TESLandTexture* FormSwapMap::GenerateLandTextureSnowVariant(const RE::TESLandTexture* a_landTexture)
{
	static constexpr std::array blackList = { "Snow"sv, "Ice"sv, "Winter"sv, "Frozen"sv, "Coast"sv, "River"sv };

	const auto editorID = edid::get_editorID(a_landTexture);
	if (!editorID.empty() && std::ranges::any_of(blackList, [&](const auto str) { return REX::STR::ICONTAINS(editorID, str); })) {
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

void FormSwapMap::LoadFormSwaps(RECORD a_record, const std::vector<std::string>& a_values)
{
	auto& map = get_map(a_record);
	for (const auto& key : a_values) {
		const auto formPair = REX::STR::SPLIT(key, "|");

		const auto formID = INI::parse_form(formPair[kBase]);
		const auto swapFormID = INI::parse_form(formPair[kSwap]);
		if (formID != 0 && swapFormID != 0) {
			map.insert_or_assign(formID, swapFormID);
		}
	}
}

void FormSwapMap::LoadFormSwaps(const CSimpleIniA& a_ini)
{
	for (const auto record : all_records()) {
		const auto name = get_name(record);

		CSimpleIniA::TNamesDepend values;
		a_ini.GetAllKeys(name.data(), values);
		values.sort(CSimpleIniA::Entry::LoadOrder());

		if (!values.empty()) {
			REX::INFO("\t\t[{}] read {} variants", name, values.size());

			std::vector<std::string> vec;
			vec.reserve(values.size());
			std::ranges::transform(values, std::back_inserter(vec), [](const auto& val) { return val.pItem; });

			LoadFormSwaps(record, vec);
		}
	}
}

//only covers winter
bool FormSwapMap::GenerateFormSwaps(CSimpleIniA& a_ini, bool a_forceRegenerate)
{
	bool save = false;

	for (const auto record : standard_records()) {
		auto type = get_name(record);

		CSimpleIniA::TNamesDepend values;
		a_ini.GetAllKeys(type.data(), values);
		values.sort(CSimpleIniA::Entry::LoadOrder());

		if (values.empty() || a_forceRegenerate) {
			save = true;

			if (a_forceRegenerate) {
				a_ini.Delete(type.data(), nullptr, true);
			}

			TempFormSwapMap map{};

			switch (record) {
			case RECORD::kLandTextures:
				get_snow_variants<RE::TESLandTexture>(a_ini, record, map);
				break;
			case RECORD::kActivators:
				get_snow_variants<RE::TESObjectACTI>(a_ini, record, map);
				break;
			case RECORD::kFurniture:
				get_snow_variants<RE::TESFurniture>(a_ini, record, map);
				break;
			case RECORD::kMovableStatics:
				get_snow_variants<RE::BGSMovableStatic>(a_ini, record, map);
				break;
			case RECORD::kStatics:
				get_snow_variants<RE::TESObjectSTAT>(a_ini, record, map);
				break;
			case RECORD::kTrees:
				get_snow_variants<RE::TESObjectTREE>(a_ini, record, map);
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
	const auto& map = get_map(RECORD::kLandTextures);
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
