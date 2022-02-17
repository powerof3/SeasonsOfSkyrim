#pragma once

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

	inline RE::FormID parse_form(const std::string& a_str)
	{
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
	}
}
