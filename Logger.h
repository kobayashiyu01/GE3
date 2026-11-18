#pragma once
#include <filesystem>
#include "StringUtility.h"
namespace Logger {
	void Log(std::ostream& os, const std::string& message);
	void ALog(const std::string& message);
	void ALog(const std::wstring& message);
};

