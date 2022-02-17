#pragma once

class FormSwapMap
{
public:
	FormSwapMap();

	enum TYPE : std::uint32_t
	{
		kBase = 0,
		kSwap
	};

	void LoadFormSwaps(const CSimpleIniA& a_ini);
	bool GenerateFormSwaps(CSimpleIniA& a_ini);

	RE::TESBoundObject* GetSwapForm(const RE::TESForm* a_form);

	RE::TESLandTexture* GetSwapLandTexture(const RE::TESForm* a_form);
	RE::TESLandTexture* GetSwapLandTextureFromTextureSet(const RE::BGSTextureSet* a_txst);

	MapPair<RE::FormID>& get_map(RE::FormType a_formType)
	{
		switch (a_formType) {
		case RE::FormType::Activator:
			return _formMap["Activators"];
		case RE::FormType::Furniture:
			return _formMap["Furniture"];
		case RE::FormType::MovableStatic:
			return _formMap["MovableStatics"];
		case RE::FormType::Static:
			return _formMap["Statics"];
		case RE::FormType::Tree:
			return _formMap["Trees"];
		case RE::FormType::Grass:
			return _formMap["Grass"];
		default:
			return _nullMap;
		}
	}

    MapPair<RE::FormID>& get_map(const std::string& a_section)
	{
		const auto it = _formMap.find(a_section);
		return it != _formMap.end() ? it->second : _nullMap;
	}

private:
	using RecordType = std::string;

    template <class T>
	using TempFormSwapMap = std::map<T*, T*>;

	static inline std::array<std::string, 7>
		formTypes{ "LandTextures", "Activators", "Furniture", "MovableStatics", "Statics", "Trees", "Grass" };

	void LoadFormSwaps_Impl(const std::string& a_type, const std::vector<std::string>& a_values);

	static RE::TESLandTexture* GenerateLandTextureSnowVariant(const RE::TESLandTexture* a_landTexture);

	template <class T>
	void get_snow_variants_by_form(RE::TESDataHandler* a_dataHandler, TempFormSwapMap<T>& a_tempFormMap);
	template <class T>
	void get_snow_variants(CSimpleIniA& a_ini, const std::string& a_type, TempFormSwapMap<T>& a_tempFormMap);

    Map<RecordType, MapPair<RE::FormID>> _formMap;

    MapPair<RE::FormID> _nullMap{};
};

template <class T>
void FormSwapMap::get_snow_variants_by_form(RE::TESDataHandler* a_dataHandler, TempFormSwapMap<T>& a_tempFormMap)
{
	auto& forms = a_dataHandler->GetFormArray(T::FORMTYPE);

	std::array blackList = { "Blacksmith"sv, "Frozen"sv, "Marker"sv };

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
				a_tempFormMap.emplace(form, snowForm);
			}
		}
	}
}

template <class T>
void FormSwapMap::get_snow_variants(CSimpleIniA& a_ini, const std::string& a_type, TempFormSwapMap<T>& a_tempFormMap)
{
	const auto dataHandler = RE::TESDataHandler::GetSingleton();

	auto& formIDMap = get_map(a_type);

	if constexpr (std::is_same_v<T, RE::TESLandTexture>) {
		for (auto& landLT : dataHandler->GetFormArray<RE::TESLandTexture>()) {
			if (const auto snowLT = GenerateLandTextureSnowVariant(landLT)) {
				a_tempFormMap.emplace(landLT, snowLT);
			}
		}
	} else if constexpr (std::is_same_v<T, RE::TESObjectSTAT>) {
		std::array snowBlackList = { "Ice"sv, "Icicle"sv, "Frozen"sv };
		std::array blackList = { "Ice"sv, "Icicle"sv, "Frozen"sv, "LoadScreen"sv, "INTERIOR"sv, "INV"sv, "DynDOLOD"sv };

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

		constexpr auto is_in_blacklist = []<auto N>(const RE::TESObjectSTAT* a_stat, const std::array<std::string_view, N>& a_blacklist)
		{
			const auto editorID = util::get_editorID(a_stat);
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
						a_tempFormMap.emplace(stat, snowStat);
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
					a_tempFormMap.emplace(tree, snowTree);
				}
			}
		}
	} else {
		get_snow_variants_by_form(dataHandler, a_tempFormMap);
	}

	for (auto& [form, swapForm] : a_tempFormMap) {
		formIDMap.emplace(form->GetFormID(), swapForm->GetFormID());

		//write values
		auto formEID = util::get_editorID(form);
		auto swapEID = util::get_editorID(swapForm);

		std::string comment = fmt::format(";{}|{}", formEID, swapEID);
		std::string value = fmt::format("0x{:X}~{}|0x{:X}~{}", form->GetLocalFormID(), form->GetFile(0)->fileName, swapForm->GetLocalFormID(), swapForm->GetFile(0)->fileName);

		a_ini.SetValue(a_type.c_str(), "", value.c_str(), comment.c_str());
	}

	logger::info("	[{}] : wrote {} variants", a_type, formIDMap.size());
}
