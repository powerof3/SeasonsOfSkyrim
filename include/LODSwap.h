#pragma once

#include <Seasons.h>

namespace LODSwap
{
	struct detail
	{
		template <class T>
		static std::string get_lod_filename()
		{
			const auto [canSwap, season] = SeasonManager::GetSingleton()->CanSwapLOD(T::type);
			return canSwap ? fmt::format(T::seasonalPath, season) : std::string(T::defaultPath);
		}
	};

	namespace Terrain
	{
		struct BuildMeshFileName
		{
			static void func(char* a_buffer, std::uint32_t a_sizeOfBuffer, const char* a_worldSpace, std::int16_t a_x, std::int16_t a_y, std::uint32_t a_scale)
			{
				const auto path = detail::get_lod_filename<BuildMeshFileName>();
				sprintf_s(a_buffer, a_sizeOfBuffer, path.c_str(), a_worldSpace, a_worldSpace, a_scale, a_x, a_y);
			}

			static inline std::size_t size = 0x39;
			static inline auto        id = RELOCATION_ID(31140, 31948);

			static inline constexpr std::string_view seasonalPath{ R"(Data\Meshes\Terrain\%s\%s.%i.%i.%i.{}.BTR)" };
			static inline constexpr std::string_view defaultPath{ R"(Data\Meshes\Terrain\%s\%s.%i.%i.%i.BTR)" };

			static inline auto type = LOD_TYPE::kTerrain;
		};

		struct BuildDiffuseTextureFileName
		{
			static void func(char* a_buffer, std::uint32_t a_sizeOfBuffer, const char* a_worldSpace, std::int16_t a_x, std::int16_t a_y, std::uint32_t a_scale)
			{
				const auto path = detail::get_lod_filename<BuildDiffuseTextureFileName>();
				sprintf_s(a_buffer, a_sizeOfBuffer, path.c_str(), a_worldSpace, a_worldSpace, a_scale, a_x, a_y);
			}

			static inline std::size_t size = 0x39;
			static inline auto        id = RELOCATION_ID(31141, 31949);

			static inline constexpr std::string_view seasonalPath{ R"(Data\Textures\Terrain\%s\%s.%i.%i.%i.{}.DDS)" };
			static inline constexpr std::string_view defaultPath{ R"(Data\Textures\Terrain\%s\%s.%i.%i.%i.DDS)" };

			static inline auto type = LOD_TYPE::kTerrain;
		};

		struct BuildNormalTextureFileName
		{
			static void func(char* a_buffer, std::uint32_t a_sizeOfBuffer, const char* a_worldSpace, std::int16_t a_x, std::int16_t a_y, std::uint32_t a_scale)
			{
				const auto path = detail::get_lod_filename<BuildNormalTextureFileName>();
				sprintf_s(a_buffer, a_sizeOfBuffer, path.c_str(), a_worldSpace, a_worldSpace, a_scale, a_x, a_y);
			}

			static inline std::size_t size = 0x39;
			static inline auto        id = RELOCATION_ID(31142, 31950);

			static inline constexpr std::string_view seasonalPath{ R"(Data\Textures\Terrain\%s\%s.%i.%i.%i.{}_n.DDS)" };
			static inline constexpr std::string_view defaultPath{ R"(Data\Textures\Terrain\%s\%s.%i.%i.%i_n.DDS)" };

			static inline auto type = LOD_TYPE::kTerrain;
		};

		inline void Install()
		{
			stl::asm_replace<BuildMeshFileName>();
			stl::asm_replace<BuildDiffuseTextureFileName>();
			stl::asm_replace<BuildNormalTextureFileName>();
		}
	}

	namespace Object
	{
		struct BuildMeshFileName
		{
			static void func(char* a_buffer, std::uint32_t a_sizeOfBuffer, const char* a_worldSpace, std::int16_t a_x, std::int16_t a_y, std::uint32_t a_scale)
			{
				const auto path = detail::get_lod_filename<BuildMeshFileName>();
				sprintf_s(a_buffer, a_sizeOfBuffer, path.c_str(), a_worldSpace, a_worldSpace, a_scale, a_x, a_y);
			}

			static inline std::size_t size = 0x39;
			static inline auto        id = RELOCATION_ID(31147, 31957);

			static inline constexpr std::string_view seasonalPath{ R"(Data\Meshes\Terrain\%s\Objects\%s.%i.%i.%i.{}.BTO)" };
			static inline constexpr std::string_view defaultPath{ R"(Data\Meshes\Terrain\%s\Objects\%s.%i.%i.%i.BTO)" };

			static inline auto type = LOD_TYPE::kObject;
		};

		struct BuildDiffuseTextureAtlasFileName
		{
			static void func(char* a_buffer, std::uint32_t a_sizeOfBuffer, const char* a_worldSpace)
			{
				const auto path = detail::get_lod_filename<BuildDiffuseTextureAtlasFileName>();
				sprintf_s(a_buffer, a_sizeOfBuffer, path.c_str(), a_worldSpace, a_worldSpace);
			}

			static inline std::size_t size = 0x1F;
			static inline auto        id = RELOCATION_ID(31148, 31958);

			static inline constexpr std::string_view seasonalPath{ R"(Data\Textures\Terrain\%s\Objects\%s.Objects.{}.DDS)" };
			static inline constexpr std::string_view defaultPath{ R"(Data\Textures\Terrain\%s\Objects\%s.Objects.DDS)" };

			static inline auto type = LOD_TYPE::kObject;
		};

		struct BuildNormalTextureAtlasFileName
		{
			static void func(char* a_buffer, std::uint32_t a_sizeOfBuffer, const char* a_worldSpace)
			{
				const auto path = detail::get_lod_filename<BuildNormalTextureAtlasFileName>();
				sprintf_s(a_buffer, a_sizeOfBuffer, path.c_str(), a_worldSpace, a_worldSpace);
			}

			static inline std::size_t size = 0x1F;
			static inline auto        id = RELOCATION_ID(31149, 31959);

			static inline constexpr std::string_view seasonalPath{ R"(Data\Textures\Terrain\%s\Objects\%s.Objects.{}_n.DDS)" };
			static inline constexpr std::string_view defaultPath{ R"(Data\Textures\Terrain\%s\Objects\%s.Objects_n.DDS)" };

			static inline auto type = LOD_TYPE::kObject;
		};

		inline void Install()
		{
			stl::asm_replace<BuildMeshFileName>();
			stl::asm_replace<BuildDiffuseTextureAtlasFileName>();
			stl::asm_replace<BuildNormalTextureAtlasFileName>();
		}
	}

	namespace Tree
	{
		struct BuildMeshFileName
		{
			static void func(char* a_buffer, std::uint32_t a_sizeOfBuffer, const char* a_worldSpace, std::int16_t a_x, std::int16_t a_y, std::uint32_t a_scale)
			{
				const auto path = detail::get_lod_filename<BuildMeshFileName>();
				sprintf_s(a_buffer, a_sizeOfBuffer, path.c_str(), a_worldSpace, a_worldSpace, a_scale, a_x, a_y);
			}

			static inline std::size_t size = 0x39;
			static inline auto        id = RELOCATION_ID(31150, 31960);

			static inline constexpr std::string_view seasonalPath{ R"(Data\Meshes\Terrain\%s\Trees\%s.%i.%i.%i.{}.BTT)" };
			static inline constexpr std::string_view defaultPath{ R"(Data\Meshes\Terrain\%s\Trees\%s.%i.%i.%i.BTT)" };

			static inline auto type = LOD_TYPE::kTree;
		};

		struct BuildTextureFileName
		{
			static void func(char* a_buffer, std::uint32_t a_sizeOfBuffer, const char* a_worldSpace)
			{
				const auto path = detail::get_lod_filename<BuildTextureFileName>();
				sprintf_s(a_buffer, a_sizeOfBuffer, path.c_str(), a_worldSpace, a_worldSpace);
			}

			static inline std::size_t size = 0x1F;
			static inline auto        id = RELOCATION_ID(31151, 31961);

			static inline constexpr std::string_view seasonalPath{ R"(Data\Textures\Terrain\%s\Trees\%sTreeLOD.{}.DDS)" };
			static inline constexpr std::string_view defaultPath{ R"(Data\Textures\Terrain\%s\Trees\%sTreeLOD.DDS)" };

			static inline auto type = LOD_TYPE::kTree;
		};

		struct BuildTypeListFileName
		{
			static void func(char* a_buffer, std::uint32_t a_sizeOfBuffer, const char* a_worldSpace)
			{
				const auto path = detail::get_lod_filename<BuildTypeListFileName>();
				sprintf_s(a_buffer, a_sizeOfBuffer, path.c_str(), a_worldSpace, a_worldSpace);
			}

			static inline std::size_t size = 0x1F;
			static inline auto        id = RELOCATION_ID(31152, 31962);

			static inline constexpr std::string_view seasonalPath{ R"(Data\Meshes\Terrain\%s\Trees\%s.{}.LST)" };
			static inline constexpr std::string_view defaultPath{ R"(Data\Meshes\Terrain\%s\Trees\%s.LST)" };

			static inline auto type = LOD_TYPE::kTree;
		};

		inline void Install()
		{
			stl::asm_replace<BuildMeshFileName>();
			stl::asm_replace<BuildTextureFileName>();
			stl::asm_replace<BuildTypeListFileName>();
		}
	}

	inline void Install()
	{
		Terrain::Install();
		Object::Install();
		Tree::Install();
	}
}
