#include "Log.h"
#include "Athena/Project/Project.h"
#include "Athena/Core/PlatformUtils.h"

#if defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif



namespace Athena
{
	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;


	void Log::Init(const LogConfig& config)
	{
		if(config.EnableConsole)
			Platform::CreateAndSyncConsole();

		spdlog::set_pattern("%^[%T] %n: %v%$");

		std::vector<spdlog::sink_ptr> logSinks;

		logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.FileLocation.string(), true));
		logSinks[0]->set_pattern("[%T] [%l] %n: %v");

		if (config.EnableConsole)
		{
			logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
			logSinks[1]->set_pattern("%^[%T] %n: %v%$");
		}

		spdlog::level::level_enum loglevel;

#ifdef ATN_LOG_LEVEL_DEBUG
		loglevel = spdlog::level::trace;
#else
		loglevel = spdlog::level::info;
#endif

		s_CoreLogger = std::make_shared<spdlog::logger>("ATHENA", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_CoreLogger);
		s_CoreLogger->set_level(loglevel);
		s_CoreLogger->flush_on(loglevel);

		s_ClientLogger = std::make_shared<spdlog::logger>("APP", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_ClientLogger);
		s_ClientLogger->set_level(loglevel);
		s_ClientLogger->flush_on(loglevel);
	}
}
