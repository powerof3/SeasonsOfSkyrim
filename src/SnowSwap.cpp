#include "SnowSwap.h"
#include "SeasonManager.h"

namespace SnowSwap
{
	void Manager::LoadSnowShaderBlacklist()
	{
		std::vector<std::string> configs;

		for (constexpr auto folder = R"(Data\Seasons)"; const auto& entry : std::filesystem::directory_iterator(folder)) {
			if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini"sv) {
				if (const auto path = entry.path().string(); path.contains("_NOSNOW")) {
					configs.push_back(path);
				}
			}
		}

		if (configs.empty()) {
			logger::warn("No .ini files with _{} suffix were found in the Data/Seasons folder. Snow Shader blacklist is not loaded");
			return;
		}

		logger::info("{} matching inis found", configs.size());

		for (auto& path : configs) {
			logger::info("	INI : {}", path);

			CSimpleIniA ini;
			ini.SetUnicode();
			ini.SetMultiKey();
			ini.SetAllowEmptyValues();

			if (const auto rc = ini.LoadFile(path.c_str()); rc < 0) {
				logger::error("	couldn't read INI");
				continue;
			}

			if (const auto values = ini.GetSection("Blacklist"); values && !values->empty()) {
				for (const auto& key : *values | std::views::keys) {
					if (auto formID = INI::parse_form(key.pItem); formID != 0) {
						_snowShaderBlacklist.insert(formID);
					}
				}
			}
		}
	}

	bool Manager::GetInSnowShaderBlacklist(const RE::TESForm* a_form) const
	{
		return _snowShaderBlacklist.contains(a_form->GetFormID());
	}

	bool Manager::IsBaseBlacklisted(const RE::TESForm* a_form) const
	{
		if (GetInSnowShaderBlacklist(a_form)) {
			return true;
		}

		std::string model = a_form->As<RE::TESModel>()->GetModel();
		return model.empty() || std::ranges::any_of(_snowShaderModelBlackList, [&](const auto& str) { return string::icontains(model, str); });
	}

	bool Manager::CanApplySnowShader(RE::TESObjectREFR* a_ref) const
	{
		if (!SeasonManager::GetSingleton()->CanApplySnowShader()) {
			return false;
		}

		if (!a_ref || a_ref->IsDeleted() || a_ref->IsInWater() || !a_ref->IsDynamicForm() && GetInSnowShaderBlacklist(a_ref)) {
			return false;
		}

		const auto base = util::get_original_base(a_ref);

		if (base != a_ref->GetBaseObject() || base->IsNot(RE::FormType::MovableStatic, RE::FormType::Container) || base->IsMarker() || base->IsHeadingMarker() || GetInSnowShaderBlacklist(base)) {
			return false;
		}

		return true;
	}

	SWAP_RESULT Manager::CanApplySnowShader(RE::TESObjectSTAT* a_static, RE::TESObjectREFR* a_ref) const
	{
		if (!SeasonManager::GetSingleton()->CanApplySnowShader()) {
			return SWAP_RESULT::kSeasonFail;
		}

		if (!a_ref || a_ref->IsDeleted() || a_ref->IsInWater() || !a_ref->IsDynamicForm() && GetInSnowShaderBlacklist(a_ref)) {
			return SWAP_RESULT::kRefFail;
		}

		const auto base = util::get_original_base(a_ref);

		if (base != a_static || a_static->IsMarker() || a_static->IsHeadingMarker() || IsBaseBlacklisted(a_static)) {
			return SWAP_RESULT::kBaseFail;
		}

		if (const auto matObject = a_static->data.materialObj; matObject && (util::is_snow_shader(matObject) || util::get_editorID(matObject).contains("Ice"sv))) {
			return SWAP_RESULT::kBaseFail;
		}

		if (a_static->IsSnowObject() || a_static->IsSkyObject() || a_static->HasTreeLOD()) {
			return SWAP_RESULT::kBaseFail;
		}

		return SWAP_RESULT::kSuccess;
	}

	SNOW_TYPE Manager::GetSnowType(RE::NiAVObject* a_node)
	{
		using Flag = RE::BSShaderProperty::EShaderPropertyFlag;

		bool hasShape = false;         //no trishapes (crash)
		bool hasInvalidShape = false;  //zero vertices/no fade node (crash)

		bool hasLightingShaderProp = true;  //no lighting prop/not skinned (crash)
		bool hasAlphaProp = false;          //no alpha prop (broken)

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

			const auto effect = a_geometry->properties[RE::BSGeometry::States::kEffect];
			const auto lightingShader = netimmerse_cast<RE::BSLightingShaderProperty*>(effect.get());
			if (!lightingShader || lightingShader->flags.any(Flag::kSkinned)) {
				hasLightingShaderProp = false;
				return RE::BSVisit::BSVisitControl::kStop;
			}

			const auto property = a_geometry->properties[RE::BSGeometry::States::kProperty];
			const auto alphaProperty = netimmerse_cast<RE::NiAlphaProperty*>(property.get());
			if (alphaProperty && (alphaProperty->GetAlphaBlending() || alphaProperty->GetAlphaTesting())) {
				hasAlphaProp = true;
				return RE::BSVisit::BSVisitControl::kStop;
			}

			return RE::BSVisit::BSVisitControl::kContinue;
		});

		if (hasShape && !hasInvalidShape && hasLightingShaderProp && !hasAlphaProp) {
			return SNOW_TYPE::kMultiPass;
		}

		return SNOW_TYPE::kSinglePass;
	}

	void Manager::ApplySinglePassSnow(RE::NiAVObject* a_node)
	{
		if (!a_node) {
			return;
		}

		auto& [init, projectedParams, projectedColor] = _defaultObj;
		if (!init) {
			const auto snowMat = GetSinglePassSnowShader();

			projectedColor = snowMat->directionalData.singlePassColor;

			projectedParams = RE::NiColorA{
				snowMat->directionalData.falloffScale,
				snowMat->directionalData.falloffBias,
				1.0f / snowMat->directionalData.noiseUVScale,
				std::cosf(RE::deg_to_rad(90.0f))
			};

			init = true;
		}

		if (a_node->SetProjectedUVData(projectedParams, projectedColor, true)) {
			ApplySnowMaterialPatch(a_node);
		}
	}

	void Manager::ApplySnowMaterialPatch(RE::NiAVObject* a_node)
	{
		if (const auto snowShaderData = RE::NiBooleanExtraData::Create("SOS_SNOW_SHADER", true)) {
			a_node->AddExtraData(snowShaderData);
		}
	}

	std::optional<Manager::SnowInfo> Manager::GetSnowInfo(const RE::TESObjectSTAT* a_static)
	{
		Locker locker(_snowInfoLock);

		if (const auto it = _snowInfoMap.find(a_static->GetFormID()); it != _snowInfoMap.end()) {
			return it->second;
		}
		return std::nullopt;
	}

	void Manager::SetSnowInfo(const RE::TESObjectSTAT* a_static, RE::BGSMaterialObject* a_originalMat, SNOW_TYPE a_snowType)
	{
		Locker locker(_snowInfoLock);

		SnowInfo snowInfo{ a_originalMat ? a_originalMat->GetFormID() : 0, a_snowType };
		_snowInfoMap.emplace(a_static->GetFormID(), snowInfo);
	}

	RE::BGSMaterialObject* Manager::GetMultiPassSnowShader()
	{
		if (!_multiPassSnowShader) {
			_multiPassSnowShader = RE::TESForm::LookupByEditorID<RE::BGSMaterialObject>("SOS_WIN_SnowMaterialObjectMP");
		}
		return _multiPassSnowShader;
	}

	RE::BGSMaterialObject* Manager::GetSinglePassSnowShader()
	{
		if (!_singlePassSnowShader) {
			_singlePassSnowShader = RE::TESForm::LookupByEditorID<RE::BGSMaterialObject>("SOS_WIN_SnowMaterialObjectSP");
		}
		return _singlePassSnowShader;
	}
}
