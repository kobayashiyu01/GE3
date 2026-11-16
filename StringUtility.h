#pragma once
#include <filesystem>
namespace stringUtility {

	// stringをwstringに変換する
	std::wstring ComverString(const std::string& str);
	// stringをstringに変換する
	std::string ComverString(const std::wstring& str);
};
class StringUtility
{};

