#include "SnowSwap.h"
#include "SeasonManager.h"

namespace SnowSwap
{
	void Manager::LoadSnowShaderSettings()
	{
		LoadSnowShaders();

		std::vector<std::string> configs;

		for (constexpr auto folder = R"(Data\Seasons)"; const auto& entry : std::filesystem::directory_iterator(folder)) {
			if (entry.is_regular_file() && entry.path().extension() == ".ini"sv) {
				const auto& path = entry.path();
				const auto& pathStem = path.stem().string();

				if (pathStem.ends_with("_SNOW") || pathStem.ends_with("_NOSNOW")) {
					configs.push_back(path.string());
				}
			}
		}

		if (configs.empty()) {
			logger::info("No .ini files with _SNOW suffix were found in Data/Seasons folder. Snow Shader settings will not be loaded");
			return;
		}

		logger::info("{} matching inis found", configs.size());

		for (auto& path : configs) {
			logger::info("\tINI : {}", path);

			CSimpleIniA ini;
			ini.SetUnicode();
			ini.SetMultiKey();
			ini.SetAllowKeyOnly();

			if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
				logger::error("\tcouldn't read INI");
				continue;
			}

			CSimpleIniA::TNamesDepend values;
			ini.GetAllKeys("Blacklist", values);
			values.sort(CSimpleIniA::Entry::LoadOrder());

			if (!values.empty()) {
				logger::info("\tReading [Blacklist]");
				for (const auto& key : values) {
					if (auto formID = INI::parse_form(key.pItem); formID != 0) {
						_snowShaderBlacklist.insert(formID);
					} else {
						logger::error("\t\tfailed to process {} [{:X}] (formID not found)", key.pItem, formID);
					}
				}
			}

			values.clear();
			ini.GetAllKeys("Multipass Snow Whitelist", values);
			values.sort(CSimpleIniA::Entry::LoadOrder());

			if (!values.empty()) {
				logger::info("	Reading [Multipass Snow Whitelist]");
				for (const auto& key : values) {
					if (std::string value = key.pItem; value.contains(R"(/)") || value.contains(R"(\)") || value.contains(".nif")) {
						_multipassSnowStrWhitelist.emplace(value);
					} else if (auto formID = INI::parse_form(value); formID != 0) {
						_multipassSnowWhitelist.emplace(formID);
					} else {
						logger::error("\t\tfailed to process {} [{:X}] (formID not found)", key.pItem, formID);
					}
				}
			}
		}
	}

	void Manager::LoadSnowShaders()
	{
		_multiPassSnowShader = RE::TESForm::LookupByEditorID<RE::BGSMaterialObject>("SOS_WIN_SnowMaterialObjectMP");
		_singlePassSnowShader = RE::TESForm::LookupByEditorID<RE::BGSMaterialObject>("SOS_WIN_SnowMaterialObjectSP");

		if (_singlePassSnowShader) {
			const auto& data = _singlePassSnowShader->directionalData;

			_defaultObj.projectedColor = data.singlePassColor;
			_defaultObj.projectedParams = RE::NiColorA{
				data.falloffScale,
				data.falloffBias,
				1.0f / data.noiseUVScale,
				0.0f
			};
		}
	}

	bool Manager::GetBlacklisted(const RE::TESForm* a_form) const
	{
		return _snowShaderBlacklist.contains(a_form->GetFormID());
	}

	bool Manager::GetBaseBlacklisted(const RE::TESForm* a_form) const
	{
		if (GetBlacklisted(a_form)) {
			return true;
		}

		const std::string_view model = a_form->As<RE::TESModel>()->GetModel();
		if (model.empty()) {
			return true;
		}

		return std::ranges::any_of(_snowShaderModelBlackList, [&](const auto& str) { return string::icontains(model, str); });
	}

	bool Manager::GetWhitelistedForMultiPassSnow(const RE::TESForm* a_form) const
	{
		if (_multipassSnowWhitelist.contains(a_form->GetFormID())) {
			return true;
		}

		const std::string_view model = a_form->As<RE::TESModel>()->GetModel();
		return std::ranges::any_of(_multipassSnowStrWhitelist, [&](const auto& str) { return string::icontains(model, str); });
	}

	SWAP_RESULT Manager::CanApplySnowShader(RE::TESObjectREFR* a_ref) const
	{
		if (!SeasonManager::GetSingleton()->CanApplySnowShader()) {
			return SWAP_RESULT::kSeasonFail;
		}

		if (!a_ref || a_ref->IsDisabled() || a_ref->IsDeleted() || a_ref->IsInWater() || !a_ref->IsDynamicForm() && GetBlacklisted(a_ref)) {
			return SWAP_RESULT::kRefFail;
		}

		if (auto invMgr = RE::Inventory3DManager::GetSingleton(); invMgr && invMgr->tempRef == a_ref) {
			return SWAP_RESULT::kRefFail;
		}

		const auto base = util::get_original_base(a_ref);

		if (base != a_ref->GetBaseObject() || base->IsNot(RE::FormType::MovableStatic, RE::FormType::Container) || base->IsMarker() || base->IsHeadingMarker() || GetBlacklisted(base)) {
			return SWAP_RESULT::kBaseFail;
		}

		/*if (raycast::is_under_shelter(a_ref)) {
			return SWAP_RESULT::kRefFail;
		}*/

		return SWAP_RESULT::kSuccess;
	}

	SWAP_RESULT Manager::CanApplySnowShader(RE::TESObjectSTAT* a_static, RE::TESObjectREFR* a_ref) const
	{
		if (!SeasonManager::GetSingleton()->CanApplySnowShader()) {
			return SWAP_RESULT::kSeasonFail;
		}

		if (!a_ref || a_ref->IsDisabled() || a_ref->IsDeleted() || a_ref->IsInWater() || !a_ref->IsDynamicForm() && GetBlacklisted(a_ref)) {
			return SWAP_RESULT::kRefFail;
		}

		const auto base = util::get_original_base(a_ref);

		if (base != a_static || a_static->IsMarker() || a_static->IsHeadingMarker() || GetBaseBlacklisted(a_static)) {
			return SWAP_RESULT::kBaseFail;
		}

		if (const auto matObject = a_static->data.materialObj; matObject && (util::is_snow_shader(matObject) || util::is_ice_shader(matObject))) {
			return SWAP_RESULT::kBaseFail;
		}

		if (a_static->IsSnowObject() || a_static->IsSkyObject() || a_static->HasTreeLOD()) {
			return SWAP_RESULT::kBaseFail;
		}

		/*if (raycast::is_under_shelter(a_ref)) {
			return SWAP_RESULT::kRefFail;
		}*/

		return SWAP_RESULT::kSuccess;
	}

	SNOW_TYPE Manager::GetSnowType(const RE::TESObjectSTAT* a_static, RE::NiAVObject* a_node) const
	{
		const auto seasonManager = SeasonManager::GetSingleton();

		using Flag = RE::BSShaderProperty::EShaderPropertyFlag;

		if (GetWhitelistedForMultiPassSnow(a_static)) {
			return SNOW_TYPE::kMultiPass;
		}

		bool hasShape = false;         // no trishapes (crash)
		bool hasInvalidShape = false;  // zero vertices/no fade node (crash)

		bool hasLightingShaderProp = true;  // no lighting prop/not skinned (crash)
		bool hasAlphaProp = false;          // no alpha prop (broken)

		RE::BSVisit::TraverseScenegraphGeometries(a_node, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
			hasShape = true;

			if (const auto shape = a_geometry->AsTriShape(); shape && shape->vertexCount == 0) {
				hasInvalidShape = true;
				return RE::BSVisit::BSVisitControl::kStop;
			}

			bool hasFadeNode = false;

			if (auto parent = a_geometry->parent; parent) {
				while (parent) {
					if (parent->AsFadeNode()) {
						hasFadeNode = true;
						break;
					}
					parent = parent->parent;
				}
			}

			if (!hasFadeNode) {
				hasInvalidShape = true;
				return RE::BSVisit::BSVisitControl::kStop;
			}

			const auto& effect = a_geometry->shaderProperty;
			const auto  lightingShader = netimmerse_cast<RE::BSLightingShaderProperty*>(effect.get());
			if (!lightingShader || lightingShader->flags.any(Flag::kSkinned)) {
				hasLightingShaderProp = false;
				return RE::BSVisit::BSVisitControl::kStop;
			}

			const auto& alphaProperty = a_geometry->alphaProperty;
			if (alphaProperty && (alphaProperty->GetAlphaBlending() || alphaProperty->GetAlphaTesting())) {
				hasAlphaProp = true;
				return RE::BSVisit::BSVisitControl::kStop;
			}

			return RE::BSVisit::BSVisitControl::kContinue;
		});

		if (seasonManager->PreferMultipass() && hasShape && !hasInvalidShape && hasLightingShaderProp && !hasAlphaProp) {
			return SNOW_TYPE::kMultiPass;
		}

		return SNOW_TYPE::kSinglePass;
	}

	void Manager::ApplySinglePassSnow(RE::NiAVObject* a_node, float a_angle) const
	{
		if (!a_node || !_singlePassSnowShader) {
			return;
		}

		RE::NiColorA projectedParams = _defaultObj.projectedParams;
		if (a_angle != 90.0f) {
			projectedParams.alpha = std::cosf(RE::deg_to_rad(a_angle));
		}

		if (a_node->SetProjectedUVData(projectedParams, _defaultObj.projectedColor, true)) {
			if (const auto snowShaderData = RE::NiBooleanExtraData::Create("SOS_SNOW_SHADER", true)) {
				a_node->AddExtraData(snowShaderData);
			}
		}
	}

	void Manager::RemoveSinglePassSnow(RE::NiAVObject* a_node) const
	{
		if (!a_node) {
			return;
		}

		using Flag = RE::BSShaderProperty::EShaderPropertyFlag;
		using Flag8 = RE::BSShaderProperty::EShaderPropertyFlag8;

		RE::BSVisit::TraverseScenegraphGeometries(a_node, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
			const auto& effect = a_geometry->shaderProperty;

			if (const auto lightingShader = netimmerse_cast<RE::BSLightingShaderProperty*>(effect.get()); lightingShader && lightingShader->flags.any(Flag::kProjectedUV, Flag::kSnow)) {
				lightingShader->SetFlags(Flag8::kProjectedUV, false);
				lightingShader->SetFlags(Flag8::kSnow, false);
			}

			return RE::BSVisit::BSVisitControl::kContinue;
		});

		a_node->RemoveExtraData("SOS_SNOW_SHADER");
	}

	std::optional<Manager::SnowInfo> Manager::GetSnowInfo(const RE::TESObjectSTAT* a_static)
	{
		ReadLocker locker(_snowInfoLock);

		if (const auto it = _snowInfoMap.find(a_static->GetFormID()); it != _snowInfoMap.end()) {
			return it->second;
		}
		return std::nullopt;
	}

	void Manager::SetSnowInfo(const RE::TESObjectSTAT* a_static, RE::BGSMaterialObject* a_originalMat, SNOW_TYPE a_snowType)
	{
		WriteLocker locker(_snowInfoLock);

		SnowInfo snowInfo{ a_originalMat ? a_originalMat->GetFormID() : 0, a_snowType };
		_snowInfoMap.emplace(a_static->GetFormID(), snowInfo);
	}
}
