#pragma once

#include <string>

class Utility final
{
public:
	static const std::string getWorkingDirectory();
	static const std::wstring to_wstring(const std::string& str);
};

