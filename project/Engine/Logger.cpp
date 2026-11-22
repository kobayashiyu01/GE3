#include "Logger.h"
#include <windows.h>
#include <string>

namespace Logger {

	void Log(std::ostream& os, const std::string& message)
	{
		os << message << std::endl;
		OutputDebugStringA(message.c_str());
	}

	void ALog(const std::string& message)
	{
		OutputDebugStringA(message.c_str());
	}

	void ALog(const std::wstring& message)
	{
		ALog(stringUtility::ConverString(message));
	}
}

