#pragma once

inline HMODULE tweaks{ nullptr };

namespace Cache
{
	class DataHolder : public ISingleton<DataHolder>
	{
	public:
		void GetData();

		RE::TESLandTexture* GetLandTextureFromTextureSet(const RE::BGSTextureSet* a_txst);
		[[nodiscard]] bool  IsSnowShader(const RE::TESForm* a_form) const;

		RE::TESBoundObject* GetOriginalBase(RE::TESObjectREFR* a_ref);
		void                SetOriginalBase(const RE::TESObjectREFR* a_ref, const RE::TESBoundObject* a_originalBase);

	private:
		using Lock = std::shared_mutex;
		using Locker = std::scoped_lock<Lock>;

		MapPair<RE::FormID> _textureToLandMap;
		Set<RE::FormID>     _snowShaders;

		mutable Lock        _originalsLock;
		MapPair<RE::FormID> _originals;
	};
}
