#pragma once
#include "MergeMapper.h"

namespace util
{
	inline std::string get_editorID(const RE::TESForm* a_form)
	{
		return Cache::DataHolder::GetSingleton()->GetEditorID(a_form->GetFormID());
	}

	inline RE::TESBoundObject* get_original_base(RE::TESObjectREFR* a_ref)
	{
		return Cache::DataHolder::GetSingleton()->GetOriginalBase(a_ref);
	}

	inline void set_original_base(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_originalBase)
	{
		Cache::DataHolder::GetSingleton()->SetOriginalBase(a_ref, a_originalBase);
	}

	inline bool is_snow_shader(const RE::BGSMaterialObject* a_shader)
	{
		return Cache::DataHolder::GetSingleton()->IsSnowShader(a_shader);
	}
}

namespace model
{
	inline bool contains_textureset(RE::TESModel* a_model, std::string_view a_txstPath)
	{
		if (const auto model = a_model->GetAsModelTextureSwap(); model && model->alternateTextures && model->numAlternateTextures > 0) {
			std::span altTextures{ model->alternateTextures, model->numAlternateTextures };
			for (const auto& textures : altTextures) {
				const auto txst = textures.textureSet;
				const std::string path = txst ? txst->textures[0].textureName.c_str() : std::string();
				if (string::icontains(path, a_txstPath)) {
					return true;
				}
			}
		}

		return false;
	}

	inline bool only_contains_textureset(RE::TESModel* a_model, const std::pair<std::string_view, std::string_view>& a_txstPath)
	{
		if (const auto model = a_model->GetAsModelTextureSwap(); model && model->alternateTextures && model->numAlternateTextures > 0) {
			std::span altTextures{ model->alternateTextures, model->numAlternateTextures };
			return std::ranges::all_of(altTextures, [&](const auto& textures) {
				const auto txst = textures.textureSet;
				const std::string path = txst ? txst->textures[0].textureName.c_str() : "";
				return string::icontains(path, a_txstPath.first) || string::icontains(path, a_txstPath.second);
			});
		}

		return true;
	}

	inline bool only_contains_textureset(RE::TESModel* a_model, std::string_view a_txstPath)
	{
		if (const auto model = a_model->GetAsModelTextureSwap(); model && model->alternateTextures && model->numAlternateTextures > 0) {
			std::span altTextures{ model->alternateTextures, model->numAlternateTextures };
			return std::ranges::all_of(altTextures, [&](const auto& textures) {
				const auto txst = textures.textureSet;
				const std::string path = txst ? txst->textures[0].textureName.c_str() : std::string();
				return string::icontains(path, a_txstPath);
			});
		}

		return false;
	}

	inline bool must_only_contain_textureset(RE::TESModel* a_model, const std::pair<std::string_view, std::string_view>& a_txstPath)
	{
		if (const auto model = a_model->GetAsModelTextureSwap(); model && model->alternateTextures && model->numAlternateTextures > 0) {
			std::span altTextures{ model->alternateTextures, model->numAlternateTextures };
			return std::ranges::all_of(altTextures, [&](const auto& textures) {
				const auto txst = textures.textureSet;
				const std::string path = txst ? txst->textures[0].textureName.c_str() : std::string();
				return string::icontains(path, a_txstPath.first) || string::icontains(path, a_txstPath.second);
			});
		}

		return false;
	}

	inline std::string& process_model_path(std::string& a_path)
	{
		if (const auto it = a_path.rfind('\\'); it != std::string::npos) {
			a_path = a_path.substr(it);
		}
		return a_path;
	}
}

namespace INI
{
	template <class T>
	void get_value(CSimpleIniA& a_ini, T& a_value, const char* a_section, const char* a_key, const char* a_comment)
	{
		a_value = string::lexical_cast<T>(a_ini.GetValue(a_section, a_key, std::to_string(stl::to_underlying(a_value)).c_str()));
		a_ini.SetValue(a_section, a_key, std::to_string(stl::to_underlying(a_value)).c_str(), a_comment);
	}

	inline void get_value(CSimpleIniA& a_ini, bool& a_value, const char* a_section, const char* a_key, const char* a_comment)
	{
		a_value = a_ini.GetBoolValue(a_section, a_key, a_value);
		a_ini.SetBoolValue(a_section, a_key, a_value, a_comment);
	}

	inline void get_value(CSimpleIniA& a_ini, std::string& a_value, const char* a_section, const char* a_key, const char* a_comment)
	{
		a_value = a_ini.GetValue(a_section, a_key, a_value.c_str());
		a_ini.SetValue(a_section, a_key, a_value.c_str(), a_comment);
	}

	inline void get_value(CSimpleIniA& a_ini, std::vector<std::string>& a_value, const char* a_section, const char* a_key, const char* a_comment, const char* a_deliminator = R"(|)")
	{
		const std::string tempValue = a_ini.GetValue(a_section, a_key, string::join(a_value, a_deliminator).c_str());
		a_value = string::split(tempValue, a_deliminator);

		a_ini.SetValue(a_section, a_key, string::join(a_value, a_deliminator).c_str(), a_comment);
	}

	inline void set_value(CSimpleIniA& a_ini, const std::vector<std::string>& a_value, const char* a_section, const char* a_key, const char* a_comment, const char* a_deliminator = R"(|)")
	{
		a_ini.SetValue(a_section, a_key, string::join(a_value, a_deliminator).c_str(), a_comment);
	}

	inline RE::FormID parse_form(const std::string& a_str)
	{
		if (a_str.find('~') != std::string::npos) {
			const auto formPair = string::split(a_str, "~");

			const auto processedFormPair = MergeMapper::GetNewFormID(formPair[1], formPair[0]);

			return RE::TESDataHandler::GetSingleton()->LookupFormID(processedFormPair.second, processedFormPair.first);
		}
		if (const auto form = RE::TESForm::LookupByEditorID(a_str); form) {
			return form->GetFormID();
		}
		return static_cast<RE::FormID>(0);
	}
}
