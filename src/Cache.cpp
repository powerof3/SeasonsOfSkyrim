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
				if (auto eid = util::get_editorID(mat); string::icontains(eid, "Snow")) {
					_snowShaders.emplace(mat->GetFormID());
				}
			}
		}
		if (const auto sosShaderSP = RE::TESForm::LookupByEditorID<RE::BGSMaterialObject>("SOS_WIN_SnowMaterialObjectSP")) {
			const auto& spColor = RE::TESForm::LookupByEditorID<RE::BGSMaterialObject>("SnowMaterialObject1P")->directionalData.singlePassColor;
			if (spColor.red != 0.0f && spColor.green != 0.0f && spColor.blue != 0.0f) {
				sosShaderSP->directionalData.singlePassColor = spColor;
			}
		}
	}

	std::string DataHolder::GetEditorID(RE::FormID a_formID)
	{
		static auto tweaks = GetModuleHandle(L"po3_Tweaks");
		static auto func = reinterpret_cast<_GetFormEditorID>(GetProcAddress(tweaks, "GetFormEditorID"));
		if (func) {
			return func(a_formID);
		}
		return {};
	}

	RE::TESLandTexture* DataHolder::GetLandTextureFromTextureSet(const RE::BGSTextureSet* a_txst)
	{
		const auto it = _textureToLandMap.find(a_txst->GetFormID());
		return RE::TESForm::LookupByID<RE::TESLandTexture>(it != _textureToLandMap.end() ? it->second : 0x00000C16);
	}

	bool DataHolder::IsSnowShader(const RE::TESForm* a_form) const
	{
		return _snowShaders.contains(a_form->GetFormID());
	}

	RE::TESBoundObject* DataHolder::GetOriginalBase(RE::TESObjectREFR* a_ref)
	{
		Locker locker(_originalsLock);

		const auto it = _originals.find(a_ref->GetFormID());
		return it != _originals.end() ? RE::TESForm::LookupByID<RE::TESBoundObject>(it->second) : a_ref->GetBaseObject();
	}

	void DataHolder::SetOriginalBase(const RE::TESObjectREFR* a_ref, const RE::TESBoundObject* a_originalBase)
	{
		Locker locker(_originalsLock);

		_originals.emplace(a_ref->GetFormID(), a_originalBase->GetFormID());
	}
}
