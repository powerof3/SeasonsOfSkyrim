#pragma once

enum class SEASON : std::uint32_t
{
	kNone = 0,
	kWinter,
	kSpring,
	kSummer,
	kAutumn
};

enum class SEASON_TYPE : std::uint32_t
{
	kOff = 0,
	kPermanentWinter,
	kPermanentSpring,
	kPermanentSummer,
	kPermanentAutumn,
	kSeasonal
};

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

	Map::FormID& get_map(RE::FormType a_formType)
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
		default:
			return _nullMap;
		}
	}
	Map::FormID& get_map(const std::string& a_section)
	{
		const auto it = _formMap.find(a_section);
		return it != _formMap.end() ? it->second : _nullMap;
	}

private:
	template <class T>
    using TempFormSwapMap = std::multimap<T*, T*>;

    static inline std::array<std::string, 6>
		formTypes{ "LandTextures", "Activators", "Furniture", "MovableStatics", "Statics", "Trees" };

	void LoadFormSwaps_Impl(const std::string& a_type, const std::vector<std::string>& a_values);

	RE::TESLandTexture* GenerateLandTextureSnowVariant(const RE::TESLandTexture* a_landTexture);

	template <class T>
	void get_snow_variants_by_form(RE::TESDataHandler* a_dataHandler, TempFormSwapMap<T>& a_tempFormMap);
	template <class T>
	void get_snow_variants(CSimpleIniA& a_ini, const std::string& a_type, TempFormSwapMap<T>& a_tempFormMap);

	Map::FormIDType _formMap;
	Map::FormID _nullMap{};
};

template <class T>
void FormSwapMap::get_snow_variants_by_form(RE::TESDataHandler* a_dataHandler, TempFormSwapMap<T>& a_tempFormMap)
{
	auto& forms = a_dataHandler->GetFormArray(T::FORMTYPE);

	std::array blackList = { "Blacksmith"sv, "Frozen"sv, "Marker"sv };

	std::map<std::string, T*> processedSnowForms;
	for (auto& baseForm : forms) {
		const auto form = skyrim_cast<T*>(baseForm);
		if (form && util::only_contains_textureset(form, "Snow"sv)) {
			std::string path = form->GetModel();
			if (path.empty()) {
				continue;
			}
			processedSnowForms.emplace(util::process_model_path(path), form);
		}
	}

	for (auto& [path, snowForm] : processedSnowForms) {
		for (auto& baseForm : forms) {
			const auto form = skyrim_cast<T*>(baseForm);
			if (form && string::icontains(form->model, path) && !util::contains_textureset(form, "Snow"sv) && !util::contains_textureset(form, "Frozen"sv)) {
				if (std::ranges::any_of(blackList, [&](const auto str) { return string::icontains(form->model, str); })) {
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
	const auto EID = Cache::DataHolder::GetSingleton();

	auto& formIDMap = get_map(a_type);

	if constexpr (std::is_same_v<T, RE::TESLandTexture>) {
		auto& landTextures = dataHandler->GetFormArray<RE::TESLandTexture>();

		for (auto& landTexture : landTextures) {
			if (const auto snowVariantLT = GenerateLandTextureSnowVariant(landTexture)) {
				a_tempFormMap.emplace(landTexture, snowVariantLT);
			}
		}
	} else if constexpr (std::is_same_v<T, RE::TESObjectSTAT>) {
		std::array blackList = { "Ice"sv, "Frozen"sv, "LoadScreen"sv, "_INTERIOR"sv, "INV"sv, "DynDOLOD"sv };

		using StaticModelMap = std::map<std::string, RE::TESObjectSTAT*>;
		StaticModelMap processedSnowStats;

		auto& statics = dataHandler->GetFormArray<RE::TESObjectSTAT>();
		for (auto& stat : statics) {
			const auto mat = stat->data.materialObj;
			if (mat && util::is_snow_shader(mat) && util::only_contains_textureset(stat, { "Snow"sv, "Mask"sv }) || util::must_only_contain_textureset(stat, { "Snow", "Mask" })) {
				std::string path = stat->GetModel();
				if (path.empty()) {
					continue;
				}
				processedSnowStats.emplace(util::process_model_path(path), stat);
			}
		}

		for (auto& [path, snowStat] : processedSnowStats) {
			for (auto& stat : statics) {
				if (string::icontains(stat->GetModel(), path) && snowStat != stat) {
					if (const auto mat = stat->data.materialObj; !mat || !util::is_snow_shader(mat)) {
						if (const auto editorID = util::get_editorID(stat); std::ranges::any_of(blackList, [&](const auto str) { return editorID.find(str) != std::string::npos; })) {
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
				if (string::icontains(tree->GetModel(), path) && !string::icontains(tree->GetModel(), "Snow")) {
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
		auto formEID = EID->GetEditorID(form->GetFormID());
		auto swapEID = EID->GetEditorID(swapForm->GetFormID());

		std::string comment = fmt::format(";{}|{}", formEID, swapEID);
		std::string value = fmt::format("0x{:X}~{}|0x{:X}~{}", form->GetLocalFormID(), form->GetFile(0)->fileName, swapForm->GetLocalFormID(), swapForm->GetFile(0)->fileName);

		a_ini.SetValue(a_type.c_str(), "", value.c_str(), comment.c_str());
	}

	logger::info("	[{}] : wrote {} variants", a_type, formIDMap.size());
}

class Season
{
public:
	explicit Season(SEASON a_season, std::pair<std::string, std::string> a_ID) :
		season(a_season),
		ID(std::move(a_ID))
	{}

	void LoadSettingsAndVerify(CSimpleIniA& a_ini, bool a_writeComment = false);

	[[nodiscard]] bool CanSwapGrass() const;
	[[nodiscard]] bool CanSwapLOD() const;
	[[nodiscard]] bool IsLandscapeSwapAllowed() const;
	[[nodiscard]] bool IsSwapAllowed(const RE::TESForm* a_form) const;

	[[nodiscard]] const std::pair<std::string, std::string>& GetID() const;
	[[nodiscard]] SEASON GetType() const;

	[[nodiscard]] FormSwapMap& GetFormSwapMap();

private:
	SEASON season{};
	std::pair<std::string, std::string> ID{};  //type, suffix (Winter, WIN)

	std::vector<std::string> allowedWorldspaces{
		"Tamriel",
		"MarkathWorld",
		"RiftenWorld",
		"SolitudeWorld",
		"WhiterunWorld",
		"DLC1HunterHQWorld",
		"DLC2SolstheimWorld"
	};

	bool swapActivators{ true };
	bool swapFurniture{ true };
	bool swapMovableStatics{ true };
	bool swapStatics{ true };
	bool swapTrees{ true };

	bool swapLOD{ true };
	bool swapGrass{ true };

	FormSwapMap formMap{};

	[[nodiscard]] bool is_valid_swap_type(const RE::TESForm* a_form) const
	{
		switch (a_form->GetFormType()) {
		case RE::FormType::Activator:
			return swapActivators;
		case RE::FormType::Furniture:
			return swapFurniture;
		case RE::FormType::MovableStatic:
			return swapMovableStatics;
		case RE::FormType::Static:
			return swapStatics;
		case RE::FormType::Tree:
			return swapTrees;
		default:
			return false;
		}
	}

    [[nodiscard]] bool is_in_valid_worldspace() const
	{
		const auto worldSpace = RE::TES::GetSingleton()->worldSpace;
		return worldSpace && std::ranges::find(allowedWorldspaces, worldSpace->GetFormEditorID()) != allowedWorldspaces.end();
	}
};
