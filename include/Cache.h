#pragma once

#include "Util.h"

namespace Cache
{
	class DataHolder
	{
	public:
		static DataHolder* GetSingleton()
		{
			static DataHolder singleton;
			return std::addressof(singleton);
		}

		void GetData();

		std::string GetEditorID(RE::FormID a_formID);

		RE::TESLandTexture* GetLandTextureFromTextureSet(const RE::TESForm* a_form);

		bool IsSnowShader(const RE::TESForm* a_form) const;

	protected:
		DataHolder() = default;
		DataHolder(const DataHolder&) = delete;
		DataHolder(DataHolder&&) = delete;
		~DataHolder() = default;

		DataHolder& operator=(const DataHolder&) = delete;
		DataHolder& operator=(DataHolder&&) = delete;

	private:
		Map::FormEditorID _formIDToEditorIDMap;
		Map::FormID _textureToLandMap;

		Set::FormID _snowShaders;
	};
}

namespace util
{
	inline std::string get_editorID(const RE::TESForm* a_form)
	{
		return Cache::DataHolder::GetSingleton()->GetEditorID(a_form->GetFormID());
	}
}
