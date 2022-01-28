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
				std::multimap<RE::TESObjectSTAT*, RE::TESObjectSTAT*> staticMap;
				get_snow_variants(a_ini, type, staticMap);
			} else if (type == "Activators") {
				std::multimap<RE::TESObjectACTI*, RE::TESObjectACTI*> activatorMap;
				get_snow_variants(a_ini, type, activatorMap);
			} else if (type == "Furniture") {
				std::multimap<RE::TESFurniture*, RE::TESFurniture*> furnitureMap;
				get_snow_variants(a_ini, type, furnitureMap);
			} else if (type == "LandTextures") {
				std::multimap<RE::TESLandTexture*, RE::TESLandTexture*> landTxstMap;
				get_snow_variants(a_ini, type, landTxstMap);
			} else if (type == "MovableStatics") {
				std::multimap<RE::BGSMovableStatic*, RE::BGSMovableStatic*> movStaticMap;
				get_snow_variants(a_ini, type, movStaticMap);
			} else if (type == "Trees") {
				std::multimap<RE::TESObjectTREE*, RE::TESObjectTREE*> treeMap;
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

RE::TESLandTexture* FormSwapMap::GetLandTexture(const RE::TESForm* a_form)
{
	const auto& map = _formMap["LandTextures"];
	if (map.empty()) {
		return nullptr;
	}

	const auto it = map.find(a_form->GetFormID());
	return it != map.end() ? RE::TESForm::LookupByID<RE::TESLandTexture>(it->second) : nullptr;
}

RE::TESLandTexture* FormSwapMap::GetLandTextureFromTextureSet(const RE::TESForm* a_form)
{
	const auto landTexture = Cache::DataHolder::GetSingleton()->GetLandTextureFromTextureSet(a_form);
	return GetLandTexture(landTexture);
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
			logger::error("LOD files for season {} not found! Please make sure seasonal LOD has been generated and installed correctly. Default LOD will be used instead", type);
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

bool Season::IsSwapAllowed() const
{
	return is_in_valid_worldspace();
}

bool Season::IsSwapAllowed(const RE::TESForm* a_form) const
{
	return IsSwapAllowed() && is_valid_swap_type(a_form);
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
