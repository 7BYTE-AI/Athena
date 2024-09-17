#pragma once

#include "Athena/Core/Core.h"

#if defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif


namespace Athena
{
	struct LogConfig
	{
		bool EnableConsole = true;
		FilePath FileLocation;	// Relative to working dir
	};

	class Log
	{
	public:
		enum class Type
		{
			Core = 1, Client
		};

		enum class Level
		{
			Trace = 1, Info, Warn, Error, Fatal
		};

	public:
		static void Init(const LogConfig& config);

		template <typename... Args>
		static void Message(Type type, Level level, std::string_view tag, Args&&... args);

	private:
		static inline String FormatMessage(const String& msg);

		template <typename... Args>
		static inline String FormatMessage(const String& msg, Args&&... args);

	private:
		ATHENA_API static std::shared_ptr<spdlog::logger> s_CoreLogger;
		ATHENA_API static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};


	template <typename... Args>
	void Log::Message(Log::Type type, Log::Level level, std::string_view tag, Args&&... args)
	{
		auto logger = type == Log::Type::Core ? s_CoreLogger : s_ClientLogger;
		std::string_view logTemplate = tag.empty() ? "{0}{1}" : "[{0}] {1}";

		String msg = FormatMessage(args...);
		auto finalMsg = fmt::vformat(logTemplate, fmt::make_format_args(tag, msg));

		switch (level)
		{
		case Log::Level::Trace:
			logger->trace(finalMsg);
			break;
		case Log::Level::Info:
			logger->info(finalMsg);
			break;
		case Log::Level::Warn:
			logger->warn(finalMsg);
			break;
		case Log::Level::Error:
			logger->error(finalMsg);
			break;
		case Log::Level::Fatal:
			logger->critical(finalMsg);
			break;
		}
	}

	inline String Log::FormatMessage(const String& msg)
	{
		return msg;
	}

	template <typename... Args>
	inline String Log::FormatMessage(const String& msg, Args&&... args)
	{
		String formattedMsg = fmt::vformat(msg, fmt::make_format_args(args...));
		return formattedMsg;
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// If defined template specialization of custom type 'Type' ToString<Type>, 
// objects of class 'Type' can be formatted
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Athena
{
	template <typename T>
	inline String ToString(const T& x)
	{
		return "Unknown type!";
	}

	template<>
	inline String ToString<std::wstring>(const std::wstring& x)
	{
		return FilePath(x).string();
	}
}

namespace fmt
{
	template <typename OStream, typename T>
	inline OStream& operator<<(OStream& os, const T& x)
	{
		return os << ::Athena::ToString(x);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tagged logs (prefer using these)
// Example with tag: ATN_TRACE_TAG("Editor", "Fatal error") "[Editor] Fatal error"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Core logging
#define ATN_CORE_TRACE_TAG(tag, ...)	::Athena::Log::Message(::Athena::Log::Type::Core, ::Athena::Log::Level::Trace, tag, __VA_ARGS__)
#define ATN_CORE_INFO_TAG(tag, ...)		::Athena::Log::Message(::Athena::Log::Type::Core, ::Athena::Log::Level::Info, tag, __VA_ARGS__)
#define ATN_CORE_WARN_TAG(tag, ...)		::Athena::Log::Message(::Athena::Log::Type::Core, ::Athena::Log::Level::Warn, tag, __VA_ARGS__)
#define ATN_CORE_ERROR_TAG(tag, ...)	::Athena::Log::Message(::Athena::Log::Type::Core, ::Athena::Log::Level::Error, tag, __VA_ARGS__)
#define ATN_CORE_FATAL_TAG(tag, ...)	::Athena::Log::Message(::Athena::Log::Type::Core, ::Athena::Log::Level::Fatal, tag, __VA_ARGS__)

// Client logging
#define ATN_TRACE_TAG(tag, ...)		::Athena::Log::Message(::Athena::Log::Type::Client, ::Athena::Log::Level::Trace, tag, __VA_ARGS__)
#define ATN_INFO_TAG(tag, ...)		::Athena::Log::Message(::Athena::Log::Type::Client, ::Athena::Log::Level::Info, tag, __VA_ARGS__)
#define ATN_WARN_TAG(tag, ...)		::Athena::Log::Message(::Athena::Log::Type::Client, ::Athena::Log::Level::Warn, tag, __VA_ARGS__)
#define ATN_ERROR_TAG(tag, ...)		::Athena::Log::Message(::Athena::Log::Type::Client, ::Athena::Log::Level::Error, tag, __VA_ARGS__)
#define ATN_FATAL_TAG(tag, ...)		::Athena::Log::Message(::Athena::Log::Type::Client, ::Athena::Log::Level::Fatal, tag, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Core logging
#define ATN_CORE_TRACE(...)		::Athena::Log::Message(::Athena::Log::Type::Core, ::Athena::Log::Level::Trace, "", __VA_ARGS__)
#define ATN_CORE_INFO(...)	    ::Athena::Log::Message(::Athena::Log::Type::Core, ::Athena::Log::Level::Info, "", __VA_ARGS__)
#define ATN_CORE_WARN(...)      ::Athena::Log::Message(::Athena::Log::Type::Core, ::Athena::Log::Level::Warn, "", __VA_ARGS__)
#define ATN_CORE_ERROR(...)     ::Athena::Log::Message(::Athena::Log::Type::Core, ::Athena::Log::Level::Error, "", __VA_ARGS__)
#define ATN_CORE_FATAL(...)     ::Athena::Log::Message(::Athena::Log::Type::Core, ::Athena::Log::Level::Fatal, "", __VA_ARGS__)

// Client logging
#define ATN_TRACE(...)		  ::Athena::Log::Message(::Athena::Log::Type::Client, ::Athena::Log::Level::Trace, "", __VA_ARGS__)
#define ATN_INFO(...)	      ::Athena::Log::Message(::Athena::Log::Type::Client, ::Athena::Log::Level::Info, "", __VA_ARGS__)
#define ATN_WARN(...)         ::Athena::Log::Message(::Athena::Log::Type::Client, ::Athena::Log::Level::Warn, "", __VA_ARGS__)
#define ATN_ERROR(...)        ::Athena::Log::Message(::Athena::Log::Type::Client, ::Athena::Log::Level::Error, "", __VA_ARGS__)
#define ATN_FATAL(...)        ::Athena::Log::Message(::Athena::Log::Type::Client, ::Athena::Log::Level::Fatal, "", __VA_ARGS__)
