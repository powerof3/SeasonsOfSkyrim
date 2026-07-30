#pragma once

namespace util
{

	inline std::size_t get_load_order_hash()
	{
		const auto dataHandler = RE::TESDataHandler::GetSingleton();

		std::size_t seed = 0;

#ifndef SKYRIMVR
		const auto& mods = dataHandler->compiledFileCollection;
		for (const auto& file : mods.files) {
			if (file) {
				boost::hash_combine(seed, file->fileName);
			}
		}
		for (const auto& file : mods.smallFiles) {
			if (file) {
				boost::hash_combine(seed, file->fileName);
			}
		}
#else
		// unverified
		if (const auto& mods = dataHandler->VRcompiledFileCollection) {
			for (const auto& file : mods->files) {
				if (file) {
					boost::hash_combine(seed, file->fileName);
				}
			}
			for (const auto& file : mods->smallFiles) {
				if (file) {
					boost::hash_combine(seed, file->fileName);
				}
			}
		} else {
			boost::hash_combine(seed, dataHandler->loadedModCount);
		}
#endif

		return seed;
	}
}

namespace model
{
	inline bool contains_textureset(RE::TESModel* a_model, std::string_view a_txstPath)
	{
		if (const auto model = a_model->GetAsModelTextureSwap(); model && model->alternateTextures && model->numAlternateTextures > 0) {
			const std::span altTextures{ model->alternateTextures, model->numAlternateTextures };

			return std::ranges::any_of(altTextures, [&](const auto& textures) {
				return textures.textureSet ? string::icontains(textures.textureSet->textures[0].textureName, a_txstPath) :
				                             false;
			});
		}

		return false;
	}

	inline bool only_contains_textureset(RE::TESModel* a_model, const std::pair<std::string_view, std::string_view>& a_txstPaths)
	{
		if (const auto model = a_model->GetAsModelTextureSwap(); model && model->alternateTextures && model->numAlternateTextures > 0) {
			const std::span altTextures{ model->alternateTextures, model->numAlternateTextures };

			return std::ranges::all_of(altTextures, [&](const auto& textures) {
				if (const auto txst = textures.textureSet) {
					return string::icontains(txst->textures[0].textureName, a_txstPaths.first) ||
					       string::icontains(txst->textures[0].textureName, a_txstPaths.second);
				}
				return false;
			});
		}

		return true;
	}

	inline bool only_contains_textureset(RE::TESModel* a_model, std::string_view a_txstPath)
	{
		if (const auto model = a_model->GetAsModelTextureSwap(); model && model->alternateTextures && model->numAlternateTextures > 0) {
			const std::span altTextures{ model->alternateTextures, model->numAlternateTextures };

			return std::ranges::all_of(altTextures, [&](const auto& textures) {
				return textures.textureSet ? string::icontains(textures.textureSet->textures[0].textureName, a_txstPath) :
				                             false;
			});
		}

		return false;
	}

	inline bool must_only_contain_textureset(RE::TESModel* a_model, const std::pair<std::string_view, std::string_view>& a_txstPaths)
	{
		if (const auto model = a_model->GetAsModelTextureSwap(); model && model->alternateTextures && model->numAlternateTextures > 0) {
			const std::span altTextures{ model->alternateTextures, model->numAlternateTextures };

			return std::ranges::all_of(altTextures, [&](const auto& textures) {
				if (const auto txst = textures.textureSet) {
					return string::icontains(txst->textures[0].textureName, a_txstPaths.first) ||
					       string::icontains(txst->textures[0].textureName, a_txstPaths.second);
				}
				return false;
			});
		}

		return false;
	}

	[[nodiscard]] inline std::string process_model_path(std::string_view a_model)
	{
		if (const auto pos = a_model.rfind('\\'); pos != std::string_view::npos) {
			a_model.remove_prefix(pos);
		}
		return string::tolower(a_model);
	}
}

namespace raycast
{
	inline bool is_under_shelter(const RE::TESObjectREFR* a_ref)
	{
		const auto cell = a_ref->GetParentCell();
		const auto bhkWorld = cell ? cell->GetbhkWorld() : nullptr;

		if (!bhkWorld) {
			return false;
		}

		RE::NiPoint3 rayStart = a_ref->GetPosition();
		RE::NiPoint3 rayEnd = rayStart;

		rayEnd.z = 9999.0f;

		RE::bhkPickData pickData;

		const auto havokWorldScale = RE::bhkWorld::GetWorldScale();
		pickData.rayInput.from = rayStart * havokWorldScale;
		pickData.rayInput.to = rayEnd * havokWorldScale;
		pickData.rayInput.enableShapeCollectionFilter = false;
		pickData.rayInput.filterInfo.SetSystemGroup(RE::bhkCollisionFilter::GetSingleton()->GetNewSystemGroup());
		pickData.rayInput.filterInfo.SetCollisionLayer(RE::COL_LAYER::kLOS);

		if (bhkWorld->PickObject(pickData); pickData.rayOutput.HasHit()) {
			if (const auto hitRef = RE::TESHavokUtilities::FindCollidableRef(*pickData.rayOutput.rootCollidable); hitRef && hitRef != a_ref) {
				return true;
			}
		}
		return false;
	}
}

namespace INI
{
	inline void set_value(CSimpleIniA& a_ini, const std::vector<std::string>& a_value, const char* a_section, const char* a_key, const char* a_comment, const char* a_deliminator = R"(|)")
	{
		a_ini.SetValue(a_section, a_key, string::join(a_value, a_deliminator).c_str(), a_comment);
	}

	inline RE::FormID parse_form(const std::string& a_str)
	{
		if (const auto splitID = string::split(a_str, "~"); splitID.size() == 2) {
			const auto  formID = string::to_num<RE::FormID>(splitID[0], true);
			const auto& modName = splitID[1];
			if (g_mergeMapperInterface) {
				const auto [mergedModName, mergedFormID] = g_mergeMapperInterface->GetNewFormID(modName.c_str(), formID);
				return RE::TESDataHandler::GetSingleton()->LookupFormID(mergedFormID, mergedModName);
			} else {
				return RE::TESDataHandler::GetSingleton()->LookupFormID(formID, modName);
			}
		}
		if (const auto form = RE::TESForm::LookupByEditorID(a_str); form) {
			return form->GetFormID();
		}
		return static_cast<RE::FormID>(0);
	}
}
