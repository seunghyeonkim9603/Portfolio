#include <fstream>
#include <Windows.h>
#include <stdarg.h>

#include "Logger.h"

thread_local WCHAR Logger::LogBuffer[Logger::LOG_BUFFER_LENGTH + 1];
thread_local unsigned int Logger::LogIndex;

void Logger::Log(const WCHAR* path)
{
	std::wofstream os;

	os.open(path, std::ios::app);
	{
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
