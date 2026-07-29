#pragma once

class FormSwapMap
{
public:
	enum TYPE : std::uint32_t
	{
		kBase = 0,
		kSwap
	};

	enum class RECORD : std::uint32_t
	{
		kLandTextures = 0,
		kActivators,
		kFurniture,
		kMovableStatics,
		kStatics,
		kTrees,

		kFlora,
		kVisualEffects,

		kTotal
	};

	void LoadFormSwaps(const CSimpleIniA& a_ini);
	void LoadFormSwaps(RECORD a_record, const std::vector<std::string>& a_values);

	bool GenerateFormSwaps(CSimpleIniA& a_ini, bool a_forceRegenerate);

	RE::TESBoundObject* GetSwapForm(const RE::TESForm* a_form);

	RE::TESLandTexture* GetSwapLandTexture(const RE::TESLandTexture* a_landTxst);
	RE::TESLandTexture* GetSwapLandTexture(const RE::BGSTextureSet* a_txst);

	[[nodiscard]] static constexpr std::string_view get_name(RECORD a_record)
	{
		return recordNames[std::to_underlying(a_record)];
	}

	[[nodiscard]] static constexpr auto all_records() { return enum_range(RECORD::kLandTextures, RECORD::kTotal); }
	[[nodiscard]] static constexpr auto standard_records() { return enum_range(RECORD::kLandTextures, RECORD::kFlora); }

	[[nodiscard]] FlatMap<RE::FormID, RE::FormID>& get_map(RECORD a_record)
	{
		return _formMaps[std::to_underlying(a_record)];
	}

	[[nodiscard]] FlatMap<RE::FormID, RE::FormID>& get_map(RE::FormType a_formType)
	{
		switch (a_formType) {
		case RE::FormType::Activator:
			return get_map(RECORD::kActivators);
		case RE::FormType::Furniture:
			return get_map(RECORD::kFurniture);
		case RE::FormType::MovableStatic:
			return get_map(RECORD::kMovableStatics);
		case RE::FormType::Static:
			return get_map(RECORD::kStatics);
		case RE::FormType::Tree:
			return get_map(RECORD::kTrees);
		case RE::FormType::Flora:
			return get_map(RECORD::kFlora);
		case RE::FormType::ReferenceEffect:
			return get_map(RECORD::kVisualEffects);
		default:
			return _nullMap;
		}
	}
	[[nodiscard]] static std::optional<RECORD> get_record(std::string_view a_section)
	{
		const auto it = std::ranges::find(recordNames, a_section);
		if (it == recordNames.end()) {
			return std::nullopt;
		}
		return static_cast<RECORD>(std::distance(recordNames.begin(), it));
	}

private:
	using TempFormSwapMap = std::map<RE::FormID, RE::FormID>;

	static constexpr std::array<std::string_view, std::to_underlying(RECORD::kTotal)> recordNames{
		"LandTextures"sv, "Activators"sv, "Furniture"sv, "MovableStatics"sv, "Statics"sv, "Trees"sv, "Flora"sv, "VisualEffects"sv
	};
	static_assert(recordNames.size() == std::to_underlying(RECORD::kTotal));

	static RE::TESLandTexture* GenerateLandTextureSnowVariant(const RE::TESLandTexture* a_landTexture);

	template <class T>
	void get_snow_variants_by_form(RE::TESDataHandler* a_dataHandler, TempFormSwapMap& a_tempFormMap);
	template <class T>
	void get_snow_variants(CSimpleIniA& a_ini, RECORD a_record, TempFormSwapMap& a_tempFormMap);

	std::array<FlatMap<RE::FormID, RE::FormID>, std::to_underlying(RECORD::kTotal)> _formMaps{};
	FlatMap<RE::FormID, RE::FormID>                                                 _nullMap{};
};

template <class T>
void FormSwapMap::get_snow_variants_by_form(RE::TESDataHandler* a_dataHandler, TempFormSwapMap& a_tempFormMap)
{
	auto& forms = a_dataHandler->GetFormArray(T::FORMTYPE);

	static constexpr std::array blackList = { "Blacksmith"sv, "Frozen"sv, "Marker"sv };

	std::map<std::string, T*> processedSnowForms;
	for (auto& baseForm : forms) {
		const auto form = skyrim_cast<T*>(baseForm);
		if (form && model::only_contains_textureset(form, "Snow"sv)) {
			std::string path = form->GetModel();
			if (path.empty()) {
				continue;
			}
			processedSnowForms.emplace(model::process_model_path(path), form);
		}
	}

	for (auto& [path, snowForm] : processedSnowForms) {
		for (auto& baseForm : forms) {
			const auto form = skyrim_cast<T*>(baseForm);
			if (form && string::icontains(form->model, path) && !model::contains_textureset(form, "Snow"sv) && !model::contains_textureset(form, "Frozen"sv)) {
				if (std::ranges::any_of(blackList, [&](const auto& str) { return string::icontains(form->model, str); })) {
					continue;
				}
				a_tempFormMap.emplace(form->GetFormID(), snowForm->GetFormID());
			}
		}
	}
}

template <class T>
void FormSwapMap::get_snow_variants(CSimpleIniA& a_ini, RECORD a_record, TempFormSwapMap& a_tempFormMap)
{
	const auto dataHandler = RE::TESDataHandler::GetSingleton();

	auto& formIDMap = get_map(a_record);

	if constexpr (std::is_same_v<T, RE::TESLandTexture>) {
		for (auto& landLT : dataHandler->GetFormArray<RE::TESLandTexture>()) {
			if (const auto snowLT = GenerateLandTextureSnowVariant(landLT)) {
				a_tempFormMap.emplace(landLT->GetFormID(), snowLT->GetFormID());
			}
		}
	} else if constexpr (std::is_same_v<T, RE::TESObjectSTAT>) {
		static constexpr std::array snowBlackList = { "Ice"sv, "Icicle"sv, "Frozen"sv };
		static constexpr std::array blackList = { "Ice"sv, "Icicle"sv, "Frozen"sv, "LoadScreen"sv, "INTERIOR"sv, "INV"sv, "DynDOLOD"sv };

		std::map<std::string, RE::TESObjectSTAT*> processedSnowStats;

		auto& statics = dataHandler->GetFormArray<RE::TESObjectSTAT>();

		if (const auto snowOverSkyrim = dataHandler->LookupModByName("SnowOverSkyrim.esp")) {
			for (auto& stat : statics) {
				if (stat && snowOverSkyrim->IsFormInMod(stat->GetFormID())) {
					std::string path = stat->GetModel();
					if (path.empty()) {
						continue;
					}
					processedSnowStats.emplace(model::process_model_path(path), stat);
				}
			}
		}

		constexpr auto is_in_blacklist = []<auto N>(const RE::TESObjectSTAT* a_stat, const std::array<std::string_view, N>& a_blacklist) {
			const auto editorID = edid::get_editorID(a_stat);
			return std::ranges::any_of(a_blacklist, [&](const auto& str) { return string::icontains(editorID, str); });
		};

		for (auto& stat : statics) {
			const auto mat = stat->data.materialObj;
			if (mat && util::is_snow_shader(mat) && model::only_contains_textureset(stat, { "Snow"sv, "Mask"sv }) || model::must_only_contain_textureset(stat, { "Snow", "Mask" })) {
				std::string path = stat->GetModel();
				if (path.empty() || is_in_blacklist(stat, snowBlackList)) {
					continue;
				}
				processedSnowStats.emplace(model::process_model_path(path), stat);
			}
		}

		for (auto& [snowPath, snowStat] : processedSnowStats) {
			for (auto& stat : statics) {
				std::string path = stat->GetModel();
				string::replace_last_instance(path, "Moss"sv, ""sv);
				if (string::icontains(path, snowPath) && snowStat != stat) {
					if (const auto mat = stat->data.materialObj; !mat || !util::is_snow_shader(mat)) {
						if (is_in_blacklist(stat, blackList)) {
							continue;
						}
						a_tempFormMap.emplace(stat->GetFormID(), snowStat->GetFormID());
					}
				}
			}
		}
	} else if constexpr (std::is_same_v<T, RE::TESObjectTREE>) {
		auto& trees = dataHandler->GetFormArray<RE::TESObjectTREE>();

		std::map<std::string, RE::TESObjectTREE*> processedSnowTrees;
		for (auto& tree : trees) {
			if (std::string path = tree->GetModel(); string::icontains(path, "Snow")) {
				string::replace_all(path, "Snow", "");
				processedSnowTrees.emplace(path, tree);
			}
		}

		for (auto& [path, snowTree] : processedSnowTrees) {
			for (auto& tree : trees) {
				if (string::icontains(tree->GetModel(), path) && tree != snowTree) {
					a_tempFormMap.emplace(tree->GetFormID(), snowTree->GetFormID());
				}
			}
		}
	} else {
		get_snow_variants_by_form<T>(dataHandler, a_tempFormMap);
	}

	const auto name = get_name(a_record);

	for (auto& [formID, swapFormID] : a_tempFormMap) {
		auto form = RE::TESForm::LookupByID(formID);
		auto swapForm = RE::TESForm::LookupByID(swapFormID);
		if (!form || !swapForm) {
			continue;
		}

		formIDMap.emplace(formID, swapFormID);

		//write values
		auto formEID = edid::get_editorID(form);
		auto swapEID = edid::get_editorID(swapForm);

		std::string comment = std::format(";{}|{}", formEID, swapEID);
		std::string value = std::format("0x{:X}~{}|0x{:X}~{}", form->GetLocalFormID(), form->GetFile(0)->fileName, swapForm->GetLocalFormID(), swapForm->GetFile(0)->fileName);

		a_ini.SetValue(name.data(), "", value.c_str(), comment.c_str());
	}

	logger::info("\t\t[{}] : wrote {} variants", name, formIDMap.size());
}
