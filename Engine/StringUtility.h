#pragma once
#include <filesystem>
namespace stringUtility {

	// stringをwstringに変換する
	std::wstring ConverString(const std::string& str);
	// stringをstringに変換する
	std::string ConverString(const std::wstring& str);
};

