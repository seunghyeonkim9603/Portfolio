#include "stdafx.h"

thread_local WCHAR Logger::LogBuffer[Logger::LOG_BUFFER_LENGTH + 1];
thread_local unsigned int Logger::LogIndex;

void Logger::Log(const WCHAR* path)
{
	std::wofstream os;
	
	WCHAR timeLog[64];

	time_t timer;
	struct tm t;
	timer = time(NULL);
	localtime_s(&t, &timer);

	swprintf_s(timeLog, L"[%d-%d-%d-%d]", t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

	os.open(path, std::ios::app);
	{
		os.write(timeLog, lstrlen(timeLog));
		os.write(LogBuffer, LogIndex);
	}
	os.close();

	LogIndex = 0;
}

void Logger::Append(const WCHAR* format, ...)
{
	va_list arg;

	va_start(arg, format);
	{
		LogIndex += vswprintf_s(&LogBuffer[LogIndex], LOG_BUFFER_LENGTH, format, arg);
	}
	va_end(arg);
}

void Logger::AppendLine(const WCHAR* format, ...)
{
	va_list arg;
	
	va_start(arg, format);
	{
		LogIndex += vswprintf_s(&LogBuffer[LogIndex], LOG_BUFFER_LENGTH, format, arg);
	}
	va_end(arg);

	LogBuffer[LogIndex++] = L'\n';
}

void Logger::Discard()
{
	LogIndex = 0;
}
