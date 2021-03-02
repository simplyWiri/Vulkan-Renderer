#include <spdlog/include/spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/include/spdlog/fmt/ostr.h>
#include <spdlog/include/spdlog/spdlog.h>

#include "Logging.h"

std::shared_ptr<spdlog::logger> TempLogger::logger;

void TempLogger::Init()
{
	std::vector<spdlog::sink_ptr> logSinks;
	logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());

	logSinks[0]->set_pattern("[%M:%S.%ems] %^[%L] >>>%$ %v");

	logger = std::make_shared<spdlog::logger>("R", std::begin(logSinks), std::end(logSinks));
	register_logger(logger);

	logger->set_level(spdlog::level::trace);
	logger->flush_on(spdlog::level::trace);
}
