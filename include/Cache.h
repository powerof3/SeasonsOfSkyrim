#pragma once

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

		static std::string GetEditorID(RE::FormID a_formID);

		RE::TESLandTexture* GetLandTextureFromTextureSet(const RE::BGSTextureSet* a_txst);

		[[nodiscard]] bool IsSnowShader(const RE::TESForm* a_form) const;

		RE::TESBoundObject* GetOriginalBase(RE::TESObjectREFR* a_ref);

		void SetOriginalBase(const RE::TESObjectREFR* a_ref, const RE::TESBoundObject* a_originalBase);

	protected:
		DataHolder() = default;
		DataHolder(const DataHolder&) = delete;
		DataHolder(DataHolder&&) = delete;
		~DataHolder() = default;

		DataHolder& operator=(const DataHolder&) = delete;
		DataHolder& operator=(DataHolder&&) = delete;

	private:
		using Lock = std::shared_mutex;
		using Locker = std::scoped_lock<Lock>;
		using _GetFormEditorID = const char* (*)(std::uint32_t);

		MapPair<RE::FormID> _textureToLandMap;
		Set<RE::FormID> _snowShaders;

		mutable Lock _originalsLock;
		MapPair<RE::FormID> _originals;
	};
}
