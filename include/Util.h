#pragma once

#include <SimpleIni.h>
#undef ERROR

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
				return textures.textureSet ? REX::STR::ICONTAINS(textures.textureSet->textures[0].textureName, a_txstPath) :
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
					return REX::STR::ICONTAINS(txst->textures[0].textureName, a_txstPaths.first) ||
					       REX::STR::ICONTAINS(txst->textures[0].textureName, a_txstPaths.second);
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
				return textures.textureSet ? REX::STR::ICONTAINS(textures.textureSet->textures[0].textureName, a_txstPath) :
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
					return REX::STR::ICONTAINS(txst->textures[0].textureName, a_txstPaths.first) ||
					       REX::STR::ICONTAINS(txst->textures[0].textureName, a_txstPaths.second);
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
		return REX::STR::TO_LOWER(a_model);
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
		a_ini.SetValue(a_section, a_key, REX::STR::JOIN(a_value, a_deliminator).c_str(), a_comment);
	}

	inline RE::FormID parse_form(const std::string& a_str)
	{
		if (const auto splitID = REX::STR::SPLIT(a_str, "~"); splitID.size() == 2) {
			const auto  formID = REX::STR::TO_NUM<RE::FormID>(splitID[0], true);
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

namespace Setting
{
	template <class T, class Base = REX::TIniSetting<T>>
	class TSetting : public Base
	{
	public:
		TSetting(std::string_view a_section, std::string_view a_key, std::string_view a_comment, T a_value) :
			Base(a_section, a_key, std::move(a_value)),
			m_section(a_section),
			m_key(a_key),
			m_comment(a_comment)
		{}

		void Save(void* a_data) override
		{
			Base::Save(a_data);

			auto& ini = *static_cast<CSimpleIniA*>(a_data);

			CSimpleIniA::TNamesDepend values;
			if (!ini.GetAllValues(m_section.data(), m_key.data(), values) || values.empty()) {
				return;
			}

			const auto&       entry = values.front();
			const std::string value{ entry.pItem };

			ini.Delete(m_section.data(), m_key.data());
			ini.SetValue(m_section.data(), m_key.data(), value.c_str(), m_comment.data());
		}

	private:
		std::string_view m_section;
		std::string_view m_key;
		std::string_view m_comment;
	};

	template <class T>
	using Setting = TSetting<T>;

	using Bool = Setting<bool>;
	using F32 = Setting<float>;
	using F64 = Setting<double>;
	using I8 = Setting<std::int8_t>;
	using I16 = Setting<std::int16_t>;
	using I32 = Setting<std::int32_t>;
	using U8 = Setting<std::uint8_t>;
	using U16 = Setting<std::uint16_t>;
	using U32 = Setting<std::uint32_t>;
	using Str = Setting<std::string>;

	template <class T = std::string>
	using Array = TSetting<T, REX::TIniSettingA<T>>;

	using StrA = Array<std::string>;
}
