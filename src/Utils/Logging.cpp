#include "Logging.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>

std::shared_ptr<spdlog::logger> TempLogger::logger;

void TempLogger::Init()
{
	std::vector<spdlog::sink_ptr> logSinks;
	logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

	logSinks[0]->set_pattern("[%M:%S.%ems] %^[%L] >>>%$ %v");

	logger = std::make_shared<spdlog::logger>("R", begin(logSinks), end(logSinks));
	spdlog::register_logger(logger);

	logger->set_level(spdlog::level::trace);
	logger->flush_on(spdlog::level::trace);
}