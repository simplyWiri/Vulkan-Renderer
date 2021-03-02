#pragma once
#include <memory>

#include <spdlog/include/spdlog/logger.h>

#undef min
#undef max

#ifndef NDEBUG

#ifdef VERBOSE
#define VerboseLog(...) TempLogger::GetLogger()->trace(__VA_ARGS__)
#else
#define VerboseLog(...) (void)0
#endif

#ifdef TRACE
#define LogInfo(...) TempLogger::GetLogger()->info(__VA_ARGS__)
#else
#define LogInfo(...) (void)0
#endif

#define Assert(term, ...) if(term) { } \
		else { LogError(__VA_ARGS__); __debugbreak(); }

#define LogWarning(...) TempLogger::GetLogger()->warn(__VA_ARGS__)
#define LogError(...) TempLogger::GetLogger()->error(__VA_ARGS__)

#else
// if we are in release mod, get rid of all logging

#define LogInfo(...) (void)0
#define LogWarning(...) (void)0
#define LogError(...) (void)0
#define VerboseLog(...) (void)0
#define Assert(term, ...) (void)0

#endif

class TempLogger
{
private:
	static std::shared_ptr<spdlog::logger> logger;

public:
	static void Init();
	static std::shared_ptr<spdlog::logger> GetLogger() { return logger; }
};
