#pragma once

namespace SnowSwap
{
	enum class SNOW_TYPE
	{
		kSinglePass = 0,
		kMultiPass
	};

	enum class SWAP_RESULT
	{
		kSeasonFail,
		kRefFail,
		kBaseFail,
		kSuccess
	};

	enum class SWAP_TYPE
	{
		kSkip = 0,
		kApply,
		kRemove
	};

	class Manager : public REX::Singleton<Manager>
	{
	public:
		struct SnowInfo
		{
			RE::FormID origShader;
			SNOW_TYPE  snowType;
		};

		struct ProjectedUV
		{
			bool         init{ false };
			RE::NiColorA projectedParams{};
			RE::NiColor  projectedColor{};
		};

		void LoadSnowShaderSettings();

		[[nodiscard]] SWAP_RESULT CanApplySnowShader(RE::TESObjectREFR* a_ref) const;
		[[nodiscard]] SWAP_RESULT CanApplySnowShader(RE::TESObjectSTAT* a_static, RE::TESObjectREFR* a_ref) const;

		[[nodiscard]] SNOW_TYPE GetSnowType(const RE::TESObjectSTAT* a_static, RE::NiAVObject* a_node) const;

		void ApplySinglePassSnow(RE::NiAVObject* a_node, float a_angle = 90.0f);
		void RemoveSinglePassSnow(RE::NiAVObject* a_node) const;

		[[nodiscard]] std::optional<SnowInfo> GetSnowInfo(const RE::TESObjectSTAT* a_static);
		void                                  SetSnowInfo(const RE::TESObjectSTAT* a_static, RE::BGSMaterialObject* a_originalMat, SNOW_TYPE a_snowType);

		[[nodiscard]] RE::BGSMaterialObject* GetMultiPassSnowShader();
		[[nodiscard]] RE::BGSMaterialObject* GetSinglePassSnowShader();

	private:
		using Lock = std::shared_mutex;
		using Locker = std::scoped_lock<Lock>;
		using SnowInfoMap = Map<RE::FormID, SnowInfo>;

		bool GetBlacklisted(const RE::TESForm* a_form) const;
		bool GetBaseBlacklisted(const RE::TESForm* a_form) const;

		bool GetWhitelistedForMultiPassSnow(const RE::TESForm* a_form) const;

		Set<RE::FormID>                            _snowShaderBlacklist{};
		Set<std::variant<RE::FormID, std::string>> _multipassSnowWhitelist{};

		mutable Lock _snowInfoLock;
		SnowInfoMap  _snowInfoMap{};

		ProjectedUV _defaultObj{};

		RE::BGSMaterialObject* _multiPassSnowShader{ nullptr };
		RE::BGSMaterialObject* _singlePassSnowShader{ nullptr };

		Set<std::string> _snowShaderModelBlackList{ R"(Effects\)", R"(Sky\)", R"(lod\)", "WetRocks", "DynDOLOD", "Marker", "Brazier" };
	};

	namespace Statics
	{
		struct Clone3D
		{
			static RE::NiAVObject* thunk(RE::TESObjectSTAT* a_static, RE::TESObjectREFR* a_ref, bool a_arg3)
			{
				const auto manager = Manager::GetSingleton();

				auto       snowInfo = manager->GetSnowInfo(a_static);
				const auto result = manager->CanApplySnowShader(a_static, a_ref);

				auto singlePassSnowState = SWAP_TYPE::kSkip;

				if (result == SWAP_RESULT::kSuccess) {
					if (snowInfo) {
						auto& [origShader, snowType] = *snowInfo;
						if (snowType == SNOW_TYPE::kMultiPass) {
							a_static->data.materialObj = manager->GetMultiPassSnowShader();
						} else {
							singlePassSnowState = SWAP_TYPE::kApply;
						}
					} else {
						if (auto tempNode = func(a_static, a_ref, a_arg3); tempNode) {
							const auto snowType = manager->GetSnowType(a_static, tempNode);
							manager->SetSnowInfo(a_static, a_static->data.materialObj, snowType);

							if (snowType == SNOW_TYPE::kMultiPass) {
								a_static->data.materialObj = manager->GetMultiPassSnowShader();

								tempNode->DeleteThis();  //refCount is zero, nothing else should touch this.
								tempNode = nullptr;

							} else {
								manager->ApplySinglePassSnow(tempNode);
								return tempNode;
							}
						}
					}
				} else if ((result == SWAP_RESULT::kSeasonFail || result == SWAP_RESULT::kRefFail) && snowInfo) {
					auto& [origShaderID, snowType] = *snowInfo;
					if (snowType == SNOW_TYPE::kMultiPass) {
						a_static->data.materialObj = origShaderID != 0 ?
						                                 RE::TESForm::LookupByID<RE::BGSMaterialObject>(origShaderID) :
						                                 nullptr;
					} else {
						singlePassSnowState = SWAP_TYPE::kRemove;
					}
				}

				const auto node = func(a_static, a_ref, a_arg3);

				if (singlePassSnowState == SWAP_TYPE::kApply) {
					manager->ApplySinglePassSnow(node, a_static->data.materialThresholdAngle);
				} else if (singlePassSnowState == SWAP_TYPE::kRemove) {
					manager->RemoveSinglePassSnow(node);
				}

				return node;
			}
			static inline REL::Relocation<decltype(thunk)> func;

			static inline constexpr std::size_t size = 0x40;
		};

		inline void Install()
		{
			stl::write_vfunc<RE::TESObjectSTAT, Clone3D>();
		}
	}

	//Movable Statics/Containers
	namespace OtherForms
	{
		struct Clone3D
		{
			static RE::NiAVObject* thunk(RE::TESBoundObject* a_base, RE::TESObjectREFR* a_ref, bool a_arg3)
			{
				const auto node = func(a_base, a_ref, a_arg3);

				const auto manager = Manager::GetSingleton();
				const auto result = manager->CanApplySnowShader(a_ref);

				if (result == SWAP_RESULT::kSuccess) {
					manager->ApplySinglePassSnow(node);
				} else if (result == SWAP_RESULT::kSeasonFail || result == SWAP_RESULT::kRefFail) {
					manager->RemoveSinglePassSnow(node);
				}

				return node;
			}
			static inline REL::Relocation<decltype(thunk)> func;
			static inline constexpr std::size_t            size = 0x40;
		};

		inline void Install()
		{
			stl::write_vfunc<RE::BGSMovableStatic, 2, Clone3D>();
			stl::write_vfunc<RE::TESObjectCONT, Clone3D>();
		}
	}

	inline void Install()
	{
		Statics::Install();
		OtherForms::Install();

		logger::info("Installed dynamic snow manager"sv);
	}
}
