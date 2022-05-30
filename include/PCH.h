#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <ShlObj.h>
#include <ranges>

#include <SimpleIni.h>
#include <fmt/format.h>
#include <robin_hood.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
namespace string = SKSE::stl::string;

using namespace std::literals;

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
		SKSE::AllocTrampoline(14);

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

template <class T1, class T2>
using Map = robin_hood::unordered_flat_map<T1, T2>;

template <class T>
using MapPair = robin_hood::unordered_flat_map<T, T>;

template <class T>
using Set = robin_hood::unordered_flat_set<T>;

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#	define OFFSET_3(se, ae, vr) ae
#elif SKYRIMVR
#	define ID(se, ae) se
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) vr
#else
#	define OFFSET(se, ae) se
#	define OFFSET_3(se, ae, vr) se
#endif

#include "Cache.h"
#include "Util.h"
#include "Version.h"
