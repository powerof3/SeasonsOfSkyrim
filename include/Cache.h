#pragma once

namespace Cache
{
	class DataHolder : public REX::Singleton<DataHolder>
	{
	public:
		void GetData();

		RE::TESLandTexture* GetLandTextureFromTextureSet(const RE::BGSTextureSet* a_txst);
		[[nodiscard]] bool  IsSnowShader(const RE::TESForm* a_form) const;
		[[nodiscard]] bool  IsIceShader(const RE::TESForm* a_form) const;

		RE::TESBoundObject* GetOriginalBase(RE::TESObjectREFR* a_ref);
		void                SetOriginalBase(const RE::TESObjectREFR* a_ref, const RE::TESBoundObject* a_originalBase);

	private:
		using Lock = std::shared_mutex;
		using ReadLocker = std::shared_lock<Lock>;
		using WriteLocker = std::unique_lock<Lock>;

		FlatMap<RE::FormID, RE::FormID> _textureToLandMap;
		FlatSet<RE::FormID>             _snowShaders;
		FlatSet<RE::FormID>             _iceShaders;

		mutable Lock                    _originalsLock;
		FlatMap<RE::FormID, RE::FormID> _originals;
	};

	RE::TESBoundObject* get_original_base(RE::TESObjectREFR* a_ref);
	void                set_original_base(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_originalBase);
	bool                is_snow_shader(const RE::BGSMaterialObject* a_shader);
	bool                is_ice_shader(const RE::BGSMaterialObject* a_shader);
}
