#pragma once

// Type of linking detection
#ifdef ATN_PLATFORM_WINDOWS
	#ifdef ATN_BUILD_DLL
		#define ATHENA_API __declspec(dllexport)
	#else
		#define  ATHENA_API __declspec(dllimport)
	#endif
#else
	#define ATHENA_API
#endif // End of linking detection


#ifdef ATN_DEBUG
	#define ATN_ENABLE_DEBUGBREAK
	#define ATN_ENABLE_ASSERTS
	#define ATN_ENABLE_VERIFIES
	#define ATN_ENABLE_PROFILING
	#define ATN_LOG_LEVEL_DEBUG
#elif ATN_RELEASE
	#define ATN_ENABLE_VERIFIES
	#define ATN_ENABLE_PROFILING
#elif ATN_DIST

#endif


#include <string>
#include <filesystem>


// Utilities
#define BIT(x) (1 << x)
#define ATN_EXPAND_MACRO(x) x
#define ATN_STRINGIFY_MACRO(x) #x

#ifndef ATN_PLATFORM_WINDOWS
	#define TEXT(quote) L##quote
#endif

// Debug break macro
#ifdef ATN_ENABLE_DEBUGBREAK
	#if defined(ATN_PLATFORM_WINDOWS)
		#define ATN_DEBUGBREAK() __debugbreak()
	#elif defined(ATN_PLATFORM_LINUX)
		#include <signal.h>
		#define ATN_DEBUGBREAK() raise(SIGTRAP)
	#else
		#define ATN_DEBUGBREAK()
	#endif
#else
	#define ATN_DEBUGBREAK()
#endif


// Asserts Implementation
#if defined(ATN_ENABLE_ASSERTS) || defined(ATN_ENABLE_VERIFIES)
	#define ATN_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { ATN##type##ERROR(msg, __VA_ARGS__); ATN_DEBUGBREAK(); } }
	#define ATN_INTERNAL_ASSERT_WITH_MSG(type, name, check, ...) ATN_INTERNAL_ASSERT_IMPL(type, check, name " failed: {0}", __VA_ARGS__)
	#define ATN_INTERNAL_ASSERT_NO_MSG(type, name, check) ATN_INTERNAL_ASSERT_IMPL(type, check, name " '{0}' failed at {1}:{2}", ATN_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define ATN_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define ATN_INTERNAL_ASSERT_GET_MACRO(...) ATN_EXPAND_MACRO( ATN_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, ATN_INTERNAL_ASSERT_WITH_MSG, ATN_INTERNAL_ASSERT_NO_MSG) )
#endif


// Asserts
#ifdef ATN_ENABLE_ASSERTS
	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define ATN_ASSERT(...) ATN_EXPAND_MACRO( ATN_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, "Assertion", __VA_ARGS__) )
	#define ATN_CORE_ASSERT(...) ATN_EXPAND_MACRO( ATN_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, "Assertion", __VA_ARGS__) )
#else
	#define ATN_ASSERT(...) 
	#define ATN_CORE_ASSERT(...)
#endif

// Verifies (More strong Asserts, works in release mode)
#ifdef ATN_ENABLE_VERIFIES
	#define ATN_VERIFY(...) ATN_EXPAND_MACRO( ATN_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, "Verify", __VA_ARGS__) )
	#define ATN_CORE_VERIFY(...) ATN_EXPAND_MACRO( ATN_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, "Verify", __VA_ARGS__) )
#else
	#define ATN_VERIFY(...)
	#define ATN_CORE_VERIFY(...)
#endif


#define ATN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }


namespace Athena
{
	using byte = ::std::uint8_t; // type of size 1 byte

	using int8  = ::std::int8_t;  // 8-bit int
	using int16 = ::std::int16_t; // 16-bit int
	using int32 = ::std::int32_t; // 32-bit int
	using int64 = ::std::int64_t; // 64-bit int

	using uint8  = ::std::uint8_t;  // 8-bit unsigned int
	using uint16 = ::std::uint16_t; // 16-bit unsigned int
	using uint32 = ::std::uint32_t; // 32-bit unsigned int
	using uint64 = ::std::uint64_t; // 64-bit unsigned int

	using String = ::std::string; // string type
	using FilePath = ::std::filesystem::path; // filepath type
}

#include "Memory.h"
#include "Profile.h"
