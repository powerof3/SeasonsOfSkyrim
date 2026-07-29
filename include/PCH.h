#pragma once

#define WIN32_LEAN_AND_MEAN

#include <ranges>
#include <shared_mutex>

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include "REX/REX/Singleton.h"

#include "MergeMapperPluginAPI.h"

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>

#include <ClibUtil/editorID.hpp>
#include <ClibUtil/simpleINI.hpp>

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
namespace string = clib_util::string;
namespace ini = clib_util::ini;
namespace edid = clib_util::editorID;

using namespace std::literals;
using namespace string::literals;
using namespace RE::literals;

namespace stl
{
	using namespace SKSE::stl;

	void asm_replace(std::uintptr_t a_from, std::size_t a_size, std::uintptr_t a_to);

	template <class T>
	void asm_replace()
	{
		REL::Relocation<std::uintptr_t> from{ T::id };
		asm_replace(from.address(), T::size, reinterpret_cast<std::uintptr_t>(T::func));
	}

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	template <class F, size_t index, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[index] };
		T::func = vtbl.write_vfunc(T::size, T::thunk);
	}

	template <class F, class T>
	void write_vfunc()
	{
		write_vfunc<F, 0, T>();
	}
}

template <class K, class D, class H = boost::hash<K>, class KEqual = std::equal_to<K>>
using FlatMap = boost::unordered_flat_map<K, D, H, KEqual>;

template <class K, class H = boost::hash<K>, class KEqual = std::equal_to<K>>
using FlatSet = boost::unordered_flat_set<K, H, KEqual>;

struct string_hash
{
	using is_transparent = void;  // enable heterogeneous overloads

	std::size_t operator()(std::string_view str) const
	{
		std::size_t seed = 0;
		for (auto it = str.begin(); it != str.end(); ++it) {
			boost::hash_combine(seed, std::tolower(*it));
		}
		return seed;
	}
};

struct string_cmp
{
	using is_transparent = void;  // enable heterogeneous overloads

	bool operator()(const std::string& str1, const std::string& str2) const
	{
		return string::iequals(str1, str2);
	}
	bool operator()(std::string_view str1, std::string_view str2) const
	{
		return string::iequals(str1, str2);
	}
};

template <class D>
using StringMap = FlatMap<std::string, D, string_hash, string_cmp>;
using StringSet = FlatSet<std::string, string_hash, string_cmp>;

constexpr inline auto enum_range(auto first, auto last)
{
	auto enum_range =
		std::views::iota(
			std::to_underlying(first),
			std::to_underlying(last)) |
		std::views::transform([](auto enum_val) {
			return (decltype(first))enum_val;
		});

	return enum_range;
};

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#	define OFFSET_3(se, ae, vr) ae
#elif SKYRIMVR
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) vr
#else
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) se
#endif

#include "Cache.h"
#include "Util.h"
#include "Version.h"
