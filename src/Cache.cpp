#include "Cache.h"

namespace Cache
{
	void DataHolder::GetData()
	{
		if (const auto dataHandler = RE::TESDataHandler::GetSingleton()) {
			for (const auto& landTexture : dataHandler->GetFormArray<RE::TESLandTexture>()) {
				if (landTexture->textureSet) {
					_textureToLandMap.emplace(landTexture->textureSet->GetFormID(), landTexture->GetFormID());
				}
			}
			for (const auto& mat : dataHandler->GetFormArray<RE::BGSMaterialObject>()) {
				const auto edid = edid::get_editorID(mat);
				if (REX::STR::ICONTAINS(edid, "Snow")) {
					_snowShaders.emplace(mat->GetFormID());
				}
				if (REX::STR::ICONTAINS(edid, "Ice")) {
					_iceShaders.emplace(mat->GetFormID());
				}
			}
		}

		const auto sosShaderSP = RE::TESForm::LookupByEditorID<RE::BGSMaterialObject>("SOS_WIN_SnowMaterialObjectSP");
		const auto snowShaderSP = RE::TESForm::LookupByEditorID<RE::BGSMaterialObject>("SnowMaterialObject1P");

		const auto spColor = snowShaderSP ? snowShaderSP->directionalData.singlePassColor : RE::NiColor();

		if (spColor != RE::NiColor()) {
			if (sosShaderSP) {
				sosShaderSP->directionalData.singlePassColor = spColor;
			}
		}
	}

	RE::TESLandTexture* DataHolder::GetLandTextureFromTextureSet(const RE::BGSTextureSet* a_txst)
	{
		const auto it = _textureToLandMap.find(a_txst->GetFormID());
		return RE::TESForm::LookupByID<RE::TESLandTexture>(it != _textureToLandMap.end() ? it->second :
																						   0x00000C16);  // LDirt
	}

	bool DataHolder::IsSnowShader(const RE::TESForm* a_form) const
	{
		return _snowShaders.contains(a_form->GetFormID());
	}

	bool DataHolder::IsIceShader(const RE::TESForm* a_form) const
	{
		return _iceShaders.contains(a_form->GetFormID());
	}

	RE::TESBoundObject* DataHolder::GetOriginalBase(RE::TESObjectREFR* a_ref)
	{
		ReadLocker locker(_originalsLock);

		const auto it = _originals.find(a_ref->GetFormID());
		return it != _originals.end() ? RE::TESForm::LookupByID<RE::TESBoundObject>(it->second) :
		                                a_ref->GetBaseObject();
	}

	void DataHolder::SetOriginalBase(const RE::TESObjectREFR* a_ref, const RE::TESBoundObject* a_originalBase)
	{
		WriteLocker locker(_originalsLock);
		_originals.emplace(a_ref->GetFormID(), a_originalBase->GetFormID());
	}

	RE::TESBoundObject* get_original_base(RE::TESObjectREFR* a_ref)
	{
		return Cache::DataHolder::GetSingleton()->GetOriginalBase(a_ref);
	}

	void set_original_base(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_originalBase)
	{
		Cache::DataHolder::GetSingleton()->SetOriginalBase(a_ref, a_originalBase);
	}

	bool is_snow_shader(const RE::BGSMaterialObject* a_shader)
	{
		return Cache::DataHolder::GetSingleton()->IsSnowShader(a_shader);
	}

	bool is_ice_shader(const RE::BGSMaterialObject* a_shader)
	{
		return Cache::DataHolder::GetSingleton()->IsIceShader(a_shader);
	}
}
