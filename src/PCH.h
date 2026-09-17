#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI

#include "RE/Skyrim.h"
#include "REX/REX.h"
#include "SKSE/SKSE.h"

#include <dxgi.h>
#include <imgui.h>

#include <ClibUtil/editorID.hpp>
#include <spdlog/sinks/basic_file_sink.h>

#include "FUCK_API.h"

#define DLLEXPORT __declspec(dllexport)

using namespace std::literals;
using namespace RE::literals;

namespace stl
{
	template <class F, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[0] };
		T::func = vtbl.write_vfunc(T::idx, T::thunk);
	}

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = REL::GetTrampoline();
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	template <class T>
	[[nodiscard]] T& setting(REX::TSetting<T>& a_setting)
	{
		return static_cast<T&>(a_setting);
	}

	template <class T>
	[[nodiscard]] const T& setting(const REX::TSetting<T>& a_setting)
	{
		return static_cast<const T&>(a_setting);
	}
}

namespace Runtime
{
	inline constexpr REL::Version SSE_1_7_99(1, 7, 99, 0);
	inline constexpr REL::Version MIN_ADDRESS_LIBRARY_V5 = SSE_1_7_99;

	[[nodiscard]] inline bool IsAtLeast1_7_99() noexcept
	{
		static bool result = REX::FModule::GetExecutingModule().GetFileVersion() >= Runtime::SSE_1_7_99;
		return result;
	}
}

#include "Version.h"

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#	define OFFSET_VERSIONED(ae, ae1799) \
		(Runtime::IsAtLeast1_7_99() ? (ae1799) : (ae))
#else
#	define OFFSET(se, ae) se
#	define OFFSET_VERSIONED(ae, ae1799) ae
#endif

#include "RE.h"
