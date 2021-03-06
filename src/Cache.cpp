#include "Cache.h"

namespace Cache
{
	void DataHolder::GetData()
	{
		const auto& [map, lock] = RE::TESForm::GetAllFormsByEditorID();
		const RE::BSReadLockGuard locker{ lock };
		if (map) {
			for (auto& [id, form] : *map) {
				switch (form->GetFormType()) {
				case RE::FormType::Activator:
				case RE::FormType::Furniture:
				case RE::FormType::MaterialObject:
				case RE::FormType::MovableStatic:
				case RE::FormType::LandTexture:
				case RE::FormType::Static:
				case RE::FormType::Tree:
				case RE::FormType::Grass:
					{
						_formIDToEditorIDMap.emplace(form->GetFormID(), id.c_str());
					}
					break;
				default:
					break;
				}
			}
		}
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
		const auto it = _formIDToEditorIDMap.find(a_formID);
		return it != _formIDToEditorIDMap.end() ? it->second : std::string();
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
