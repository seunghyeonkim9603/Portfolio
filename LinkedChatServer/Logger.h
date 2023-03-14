#pragma once

class Logger final
{
public:
	Logger() = delete;
	~Logger() = delete;
	Logger(Logger& other) = delete;

	Logger& operator=(Logger& other) = delete;

	static void Log(const WCHAR* path);
	static void Append(const WCHAR* format, ...);
	static void AppendLine(const WCHAR* format, ...);
	static void Discard();

private:
	enum { LOG_BUFFER_LENGTH = 4096 };

	static thread_local WCHAR			LogBuffer[LOG_BUFFER_LENGTH + 1];
	static thread_local unsigned int	LogIndex;
};
