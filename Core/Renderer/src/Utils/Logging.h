#pragma once

#ifndef NDEBUG

template <typename... Args>
constexpr void Log(const char* s, const Args&... args)
{
	printf(s, args...);
	printf("\n");
};

template <typename... Args>
constexpr void Warn(const char* s, const Args&... args)
{
	printf(s, args...);
	printf("\n");
};

template <typename... Args>
constexpr void Error(const char* s, const Args&... args)
{
	printf(s, args...);
	printf("\n");
};

template <typename... Args>
constexpr void Assert(bool cond, const char* s, const Args&... args)
{
	if (!cond)
	{
		printf(s, args...);
		printf("\n");
		__debugbreak();
	}
};

#else

template <typename... Args>
void Log(const char* s, const Args&... args)
{
}

template <typename... Args>
void Warn(const char* s, const Args&... args)
{

};

template <typename... Args>
void Error(const char* s, const Args&... args)
{
}

template <typename... Args>
void Assert(bool cond, const char* s, const Args&... args)
{
}
#endif
